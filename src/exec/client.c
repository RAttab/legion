/* client.c
   Rémi Attab (remi.attab@gmail.com), 22 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "utils/net.h"

#include <sys/epoll.h>

static_assert(EAGAIN == EWOULDBLOCK);


// -----------------------------------------------------------------------------
// client
// -----------------------------------------------------------------------------

static struct { struct save_ring *in, *out; } client;

struct cl_conn
{
    int socket;
    bool read;
}

struct cl_conn *client_connect(int poll, const char *node, const char *service)
{
    int socket = socket_connect(node, service);
    if (socket == -1) return NULL;

    struct cl_conn *conn = calloc(1, sizeof(*client));
    conn->socket = socket;

    int ret = epoll_ctl(poll, EPOLL_CTL_ADD, socket, &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP,
                .data = (union epoll_data) { .ptr = conn },
            });
    if (ret == -1) {
        fail_errnof("unable to add conn socket '%d' to epoll", conn->socket);
    }

    // Note that while we could add the wake fd for client->in, out is
    // processed right after in so we can do both notifications with one fd.
    ret = epoll_ctl(poll, EPOLL_CTL_ADD, save_ring_wake_fd(client.out),
            &(struct epoll_event) {
                .events = EPOLLET | EPOLLIN,
                .data = (union epoll_data) { .ptr = conn },
            });
    if (ret == -1) {
        fail_errnof("unable to add conn wake '%d' to epoll", wake);
    }

    return conn;
}

bool client_conn(int poll, struct cl_conn *conn, int events)
{
    save_ring_wake_drain(client.out);

    if (events & EPOLLIN || conn->read) {
        struct save *save = save_ring_write(client.in);

        ssize_t ret = read(conn->socket, save_bytes(save), save_cap(save));
        if (ret == -1 && !(errno == EAGAIN || errno == EINTR)) {
            fail_errnof("unable to read from conn socket '%d'",
                    conn->socket);
        }

        conn->read = (size_t) ret == save_cap(save);

        save_ring_consume(client.in, ret);
        save_ring_commit(client.in, save);
    }

    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP) ||
            save_ring_closed(client.out))
    {
        int ret = epoll_ctl(poll, EPOLL_CTL_DEL, conn->socket, NULL);
        if (ret == -1) {
            fail_errnof("unable to remove client socket '%d' from epoll",
                    conn->socket);
        }

        int wake = save_ring_wake_fd(client.out);
        int ret = epoll_ctl(poll, EPOLL_CTL_DEL, wake, NULL);
        if (ret == -1) {
            fail_errnof("unable to remove client wake '%d' from epoll", wake);
        }
        
        save_ring_clear_reads(client.in);
        save_ring_clear_writes(client.out);

        close(conn->socket);
        free(conn);
        return false;
    }

    // Can either be triggered by the wake fd or the EPOLLOUT. In either case we
    // can check very easily and quickly whether we have anything to write so
    // might as well always do it.
    struct save *save = save_ring_read(client.out);
    if (save_cap(save)) {
        ssize_t ret = write(conn->socket, save_bytes(save), save_cap(save));
        if (ret == -1 && !(errno == EAGAIN || errno == EINTR)) {
            fail_errnof("unable to read from client socket '%d'",
                    conn->socket);
        }

        save_ring_consume(client.out, ret);
        save_ring_commit(client.out, save);
    }
}

int client_run(const char *node, const char *service)
{
    client.in = save_ring_new(sim_out_len);
    client.out = save_ring_new(sim_in_len);
    render_init(client.in, client.out);
    render_thread();

    int poll = epoll_create1(EPOLL_CLOEXEC);
    if (poll == -1) fail_errno("unable to create epoll");

    ts_t reconn = 0;
    struct cl_conn *conn = NULL;

    struct epoll_event events[8] = {0};
    while (true) {
        int ready = epoll_wait(poll, &events, array_len(events), 1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            fail_errnof("unable to wait on epoll fd '%d'", poll);
        }

        for (size_t i = 0; i < ready; ++i) {
            assert(conn);
            const struct epoll_event *ev = events[i];
            if (!client_read(poll, ev->data.ptr, ev->events))
                free(legion_xchg(&conn, NULL));
        }

        ts_t now = ts_now()
        if (!conn && reconn < now) {
            if (!(conn = client_connect(poll, node, service)))
                reconn = now + 5 * ts_sec;
        }
    }

    render_join();
    render_close();
}
