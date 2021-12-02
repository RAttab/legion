/* server.c
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/net.h"

#include <sys/epoll.h>

static_assert(EAGAIN == EWOULDBLOCK);


// -----------------------------------------------------------------------------
// server
// -----------------------------------------------------------------------------

static struct { struct sim *sim; } server;

struct sv_client
{
    int socket;
    bool read;

    struct sockaddr_storage addr;
    socklen_t addr_len;

    struct sim_pipe *pipe;
    struct save_ring *in, *out;
};

static void server_accept(int poll, int listen)
{
    struct sockaddr_storage addr = {0};
    while (true) {
        socklen_t addr_len = sizeof(addr);
        int socket = accept4(listen, &addr, &addr_len, SOCK_NONBLOCK);
        if (socket == -1) {
            if (errno == EAGAIN) return;
            fail_errno("unable to accept connection");
        }

        struct sv_client *client = calloc(1, sizeof(*client));
        *client = (struct client) {
            .socket = socket,
            .read = false,
            .addr = addr,
            .addr_len = addr_len,
            .pipe = sim_pipe_new(server.sim),
        };
        client->in = sim_pipe_in(client->pipe);
        client->out = sim_pipe_out(client->pipe);

        int ret = epoll_ctl(poll, EPOLL_CTL_ADD, socket, &(struct epoll_event) {
                    .events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP,
                    .data = (union epoll_data) { .ptr = client },
                });
        if (ret == -1) {
            fail_errnof("unable to add client socket '%d' to epoll",
                    client->socket);
        }

        // Note that while we could add the wake fd for client->in, out is
        // processed right after in so we can do both notifications with one fd.
        ret = epoll_ctl(poll, EPOLL_CTL_ADD, save_ring_wake_fd(client->out),
                &(struct epoll_event) {
                    .events = EPOLLET | EPOLLIN,
                    .data = (union epoll_data) { .ptr = client },
                });
        if (ret == -1) {
            fail_errnof("unable to add client wake '%d' to epoll",
                    wake);
        }
    }
}

static void server_client(int poll, struct sv_client *client, uint32_t events)
{
    save_ring_wake_drain(client->out);

    if (events & EPOLLIN || client->read) {
        struct save *save = save_ring_write(client->in);

        ssize_t ret = read(client->socket, save_bytes(save), save_cap(save));
        if (ret == -1 && !(errno == EAGAIN || errno == EINTR)) {
            fail_errnof("unable to read from client socket '%d'",
                    client->socket);
        }

        client->read = (size_t) ret == save_cap(save);

        save_ring_consume(client->in, ret);
        save_ring_commit(client->in, save);
    }

    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ||
            save_ring_closed(client->out))
    {
        int ret = epoll_ctl(poll, EPOLL_CTL_DEL, client->socket, NULL);
        if (ret == -1) {
            fail_errnof("unable to remove client socket '%d' from epoll",
                    client->socket);
        }

        int wake = save_ring_wake_fd(client->out);
        int ret = epoll_ctl(poll, EPOLL_CTL_DEL, wake, NULL);
        if (ret == -1) {
            fail_errnof("unable to remove client wake '%d' from epoll", wake);
        }

        sim_pipe_close(client->pipe);
        close(client->socket);
        free(client);
        return;
    }

    // Can either be triggered by the wake fd or the EPOLLOUT. In either case we
    // can check very easily and quickly whether we have anything to write so
    // might as well always do it.
    struct save *save = save_ring_read(client->out);
    if (save_cap(save)) {
        ssize_t ret = write(client->socket, save_bytes(save), save_cap(save));
        if (ret == -1 && !(errno == EAGAIN || errno == EINTR)) {
            fail_errnof("unable to read from client socket '%d'",
                    client->socket);
        }

        save_ring_consume(client->out, ret);
        save_ring_commit(client->out, save);
    }
}

int server_run(const char *node, const char *service)
{
    server.sim = sim_load();
    sim_thread();

    int poll = epoll_create1(EPOLL_CLOEXEC);
    if (poll == -1) fail_errno("unable to create epoll");

    int listen = socket_listen(node, service);
    if (listen == -1) fail("unable to create listen socket");

    int ret = epoll_ctl(poll, EPOLL_CTL_ADD, &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN | EPOLLERR,
                .data = (union epoll_data) { .fd = listen },
            });
    if (ret == -1) {
        fail_errnof("unable to add listen fd '%d', to epoll", listen);
    }

    struct epoll_event events[8] = {0};
    while (true) {
        int ready = epoll_wait(poll, &events, array_len(events), 1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            fail_errnof("unable to wait on epoll fd '%d'", poll);
        }

        for (size_t i = 0; i < ready; ++i) {
            const struct epoll_event *ev = events[i];
            if (ev->data.fd == listen)
                server_accept(poll, listen);
            else server_client(poll, ev->data.ptr, ev->events);
        }
    }

    sim_join();
    sim_free();
}
