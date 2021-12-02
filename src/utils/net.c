/* net.c
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/net.h"
#include "utils/err.h"

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>


// -----------------------------------------------------------------------------
// socket
// -----------------------------------------------------------------------------

enum { socket_listen_backlog = 64 };

static bool socket_nodelay(int fd)
{
    int value = 1;
    int err = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    if (!err) return true;

    errf_errno("unable to disable naggle algorithm on fd '%d'", fd);
    return false;
}

int socket_listen(const char *node, const char *service)
{
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_hinnts = AI_PASSIVE | AI_ADDRCONFIG,
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
        if (bind(fd, &it->ai_addr, it->ai_addrlen) == -1) goto fail;
        if (listen(fd, socket_listen_backlog) == -1) {
            errf_errno("unable to listen on fd '%d' for node '%s' and service '%s'",
                    fd, node, service);
            goto fail;
        }

        result = fd;
        break;

      fail:
        close(fd);
    }

    freeaddrinfo(it);

    if (result == -1) {
        err("unable to bind on host '%s' and service '%s'", node, service);
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
        .ai_hinnts = AI_ADDRCONFIG,
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
        if (connect(fd, &it->ai_addr, it->ai_addrlen) == -1) goto fail;

        result = fd;
        break;

      fail:
        close(fd);
    }

    freeaddrinfo(it);

    if (result == -1) {
        failf("unable to connect to node '%s' on service '%s'", node, service);
        return -1;
    }

    return result;
}
