/* net.c
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/net.h"
#include "utils/err.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>


// -----------------------------------------------------------------------------
// socket
// -----------------------------------------------------------------------------

constexpr size_t socket_listen_backlog = 64;

static bool socket_nodelay(int fd)
{
    int value = 1;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    if (ret == 0) return true;

    errf_errno("unable to disable naggle algorithm on fd '%d'", fd);
    return false;
}

int socket_listen(const char *node, const char *service)
{
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_flags = AI_PASSIVE | AI_ADDRCONFIG,
    };

    struct addrinfo *list = NULL;
    int err = getaddrinfo(node, service, &hints, &list);
    if (err) {
        errf_num(err, gai_strerror(err),
                "unable to get address info for node '%s' and service '%s'",
                node, service);
        return -1;
    }

    int result = -1;
    for (struct addrinfo *it = list; it; it = it->ai_next) {
        const int flags = SOCK_NONBLOCK;
        int fd = socket(it->ai_family, it->ai_socktype | flags, it->ai_protocol);
        if (fd == -1) continue;

        if (!socket_nodelay(fd)) goto fail;
        if (bind(fd, it->ai_addr, it->ai_addrlen) == -1) goto fail;
        if (listen(fd, socket_listen_backlog) == -1) goto fail;

        result = fd;
        break;

      fail:
        close(fd);
    }

    freeaddrinfo(list);

    if (result == -1) {
        errf("unable to listen on socket for '%s:%s'", node, service);
        return -1;
    }

    return result;
}

int socket_connect(const char *node, const char *service)
{
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_flags = AI_ADDRCONFIG,
    };

    struct addrinfo *list = NULL;
    int err = getaddrinfo(node, service, &hints, &list);
    if (err) {
        errf_num(err, gai_strerror(err),
                "unable to get address info for node '%s' and service '%s'",
                node, service);
        return -1;
    }

    int result = -1;
    for (struct addrinfo *it = list; it; it = it->ai_next) {
        const int flags = SOCK_NONBLOCK;
        int fd = socket(it->ai_family, it->ai_socktype | flags, it->ai_protocol);
        if (fd == -1) continue;

        if (!socket_nodelay(fd)) goto fail;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == -1) {
            if (errno != EINPROGRESS) goto fail;
        }

        result = fd;
        break;

      fail:
        close(fd);
    }

    freeaddrinfo(list);

    if (result == -1) {
        errf("unable to connect socket to '%s:%s'", node, service);
        return -1;
    }

    return result;
}


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

#include <arpa/inet.h>

struct sockaddr_str sockaddr_str(struct sockaddr *addr)
{
    struct sockaddr_str str = {0};

    const char *ret = NULL;
    switch (addr->sa_family)
    {

    case AF_INET: {
        ret = inet_ntop(
                AF_INET, &((struct sockaddr_in *) addr)->sin_addr,
                str.c, sizeof(str.c));
        break;
    }

    case AF_INET6: {
        ret = inet_ntop(
                AF_INET, &((struct sockaddr_in6 *) addr)->sin6_addr,
                str.c, sizeof(str.c));
        break;
    }

    default: { assert(false); }
    }

    if (!ret) fail_errno("unable to convert sockaddr to string");
    return str;
}


// -----------------------------------------------------------------------------
// sigintfd
// -----------------------------------------------------------------------------
// I just wanted to say that signals live in a world that is divorsed from logic
// and that I gave up trying to make sense of them. The following code is the
// result of brute-forced empirical testing.

#include <sys/signalfd.h>
#include <signal.h>


int sigintfd_new(void)
{
    // Blocking the signal doesn't seem to be enough despite what the man-page
    // says. This is true in the server where GLFW is not at all involved.
    struct sigaction act = { .sa_handler = SIG_IGN };
    struct sigaction prev = {0};
    assert(!sigaction(SIGINT, &act, &prev));
    assert(prev.sa_handler == SIG_DFL);

    sigset_t set = {0};
    if (sigemptyset(&set) == -1)
        fail_errno("unable to initialize sigset");
    if (sigaddset(&set, SIGINT) == -1)
        fail_errno("unable to add SIGINT to sigset");

    sigset_t old = {0};
    if (sigprocmask(SIG_BLOCK, &set, &old) == -1)
        fail_errno("unable to block SIGINT for the process");
    assert(sigismember(&old, SIGINT) == 0);

    int fd = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd == -1) err_errno("unable to create fd for sigint");
    return fd;
}

void sigintfd_close(int fd)
{
    struct sigaction prev = {0};
    struct sigaction act = { .sa_handler = SIG_DFL };
    assert(!sigaction(SIGINT, &act, &prev));
    assert(prev.sa_handler == SIG_IGN);

    sigset_t set = {0};
    if (sigemptyset(&set) == -1)
        fail_errno("unable to initialize sigset");
    if (sigaddset(&set, SIGINT) == -1)
        fail_errno("unable to add SIGINT to sigset");

    sigset_t old = {0};
    if (sigprocmask(SIG_UNBLOCK, &set, &old) == -1)
        fail_errno("unable to block SIGINT for the process");
    assert(sigismember(&old, SIGINT) == 1);

    // Unclear whether we should unblock before closing or vice-versa. I think
    // there are race conditions either way. For our use cases, it doesn't
    // really matter though as this function is a formality before we exit the
    // process.
    close(fd);
}

bool sigintfd_read(int fd)
{
    struct signalfd_siginfo info = {0};

    ssize_t ret = read(fd, &info, sizeof(info));
    if (ret == -1) {
        // Should really return false but empirical testing has left me
        // completely and uterly confused. As such we're going to rely on the fd
        // being triggered in epoll and kinda ignore the fact that read is as
        // confused as I am. It's bad and makes no logical sense but I don't
        // want to live in the world that signalfd is trying to give me. My only
        // remaining option is to reject its reality and substitute my own.
        if (errno == EAGAIN) return true;

        failf_errno("unable to read from sigintfd '%d'", fd);
    }

    if (ret == 0) return false;

    assert(ret == sizeof(info));
    assert(info.ssi_signo == SIGINT);
    return true;
}
