/* client.c
   Rémi Attab (remi.attab@gmail.com), 22 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "game/proxy.h"
#include "render/render.h"
#include "utils/net.h"
#include "utils/time.h"
#include "utils/sdl.h"

#include <sys/epoll.h>

static_assert(EAGAIN == EWOULDBLOCK);


// -----------------------------------------------------------------------------
// client
// -----------------------------------------------------------------------------

struct server
{
    int socket;
    bool read;

    struct proxy_pipe *pipe;
    struct save_ring *in, *out;
};

static struct
{
    struct proxy *proxy;
} client;


static void client_free(int poll, struct server *server)
{
    int ret = epoll_ctl(poll, EPOLL_CTL_DEL, server->socket, NULL);
    if (ret == -1) {
        failf_errno("unable to remove client socket '%d' from epoll",
                server->socket);
    }

    int wake = save_ring_wake_fd(server->out);
    ret = epoll_ctl(poll, EPOLL_CTL_DEL, wake, NULL);
    if (ret == -1) {
        failf_errno("unable to remove client wake '%d' from epoll", wake);
    }

    proxy_pipe_close(client.proxy, server->pipe);
    close(server->socket);
    free(server);
}

static struct server *client_connect(
        int poll, const char *node, const char *service)
{
    int socket = socket_connect(node, service);
    if (socket == -1) return NULL;

    struct server *server = calloc(1, sizeof(*server));
    server->socket = socket;
    server->pipe = proxy_pipe_new(client.proxy, NULL);
    server->in = proxy_pipe_in(server->pipe);
    server->out = proxy_pipe_out(server->pipe);

    int ret = epoll_ctl(poll, EPOLL_CTL_ADD, socket, &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP,
                .data = (union epoll_data) { .ptr = server },
            });
    if (ret == -1) {
        failf_errno("unable to add server socket '%d' to epoll",
                server->socket);
    }

    // Note that while we could add the wake fd for client->in, out is
    // processed right after in so we can do both notifications with one fd.
    int wake = save_ring_wake_fd(server->out);
    ret = epoll_ctl(poll, EPOLL_CTL_ADD, wake, &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN,
                .data = (union epoll_data) { .ptr = server },
            });
    if (ret == -1) {
        failf_errno("unable to add server wake '%d' to epoll", wake);
    }

    return server;
}

static bool client_events(int poll, struct server *server, int events)
{
    save_ring_wake_drain(server->out);

    if (events & EPOLLIN || server->read) {
        struct save *save = save_ring_write(server->in);

        ssize_t ret = read(server->socket, save_bytes(save), save_cap(save));
        if (ret == -1) {
            switch (errno) {
            case ECONNREFUSED: { events |= EPOLLHUP; } // fallthrough
            case EAGAIN: case EINTR: case ECONNRESET:  { ret = 0; break; }
            default: {
                failf_errno("unable to read from server socket '%d'",
                        server->socket);
                break;
            }
            }
        }

        server->read = (size_t) ret == save_cap(save);

        save_ring_consume(save, ret);
        save_ring_commit(server->in, save);
    }

    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ||
            save_ring_closed(server->out))
    {
        client_free(poll, server);
        return false;
    }

    // Can either be triggered by the wake fd or the EPOLLOUT. In either case we
    // can check very easily and quickly whether we have anything to write so
    // might as well always do it.
    struct save *save = save_ring_read(server->out);
    if (save_cap(save)) {
        ssize_t ret = write(server->socket, save_bytes(save), save_cap(save));
        if (ret == -1 && !(errno == EAGAIN || errno == EINTR)) {
            failf_errno("unable to read from client socket '%d'",
                    server->socket);
        }

        save_ring_consume(save, ret);
        save_ring_commit(server->out, save);
    }

    return true;
}

bool client_run(const char *node, const char *service)
{
    client.proxy = proxy_new();

    sdl_disable_signals();
    render_init(client.proxy);
    render_fork();

    int poll = epoll_create1(EPOLL_CLOEXEC);
    if (poll == -1) fail_errno("unable to create epoll");

    int sigint = sigintfd_new();
    if (sigint == -1) return false;

    int ret = epoll_ctl(poll, EPOLL_CTL_ADD, sigint, &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN,
                .data = (union epoll_data) { .fd = sigint },
            });
    if (ret == -1) failf_errno("unable to add sigint fd '%d' to epoll", sigint);

    struct { ts_t ts; size_t inc; } reconn = { .ts = 0, .inc = 1 };

    bool exit = false;
    bool connected = false;
    struct server *server = NULL;
    struct epoll_event events[8] = {0};

    infof("connecting to '%s:%s'", node, service);

    while (!exit && !render_done()) {
        int ready = epoll_wait(poll, events, array_len(events), 1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            failf_errno("unable to wait on epoll fd '%d'", poll);
        }

        for (size_t i = 0; i < (size_t) ready; ++i) {
            const struct epoll_event *ev = &events[i];

            if (ev->data.fd == sigint) {
                if (sigintfd_read(sigint)) exit = true;
                continue;
            }

            assert(server && server == ev->data.ptr);
            if (client_events(poll, ev->data.ptr, ev->events)) {
                if (!connected) infof("connected to '%s:%s'", node, service);
                connected = true;
                reconn.inc = 1;
            }
            else {
                if (connected) infof("disconnected from '%s:%s'", node, service);
                connected = false;
                server = NULL;
            }
        }

        if (!server) {
            ts_t now = ts_now();
            if (now < reconn.ts) continue;
            reconn.inc = legion_min(reconn.inc * 2, (size_t) 60);
            reconn.ts = (reconn.inc * ts_sec) + now;

            infof("reconnecting to '%s:%s'", node, service);
            server = client_connect(poll, node, service);
        }
    }

    info("shutting down");

    if (server) client_free(poll, server);
    sigintfd_close(sigint);
    close(poll);

    render_join();
    render_close();

    return true;
}
