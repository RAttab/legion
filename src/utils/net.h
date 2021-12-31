/* net.h
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include <sys/socket.h>
#include <arpa/inet.h>


// -----------------------------------------------------------------------------
// socket
// -----------------------------------------------------------------------------

int socket_listen(const char *node, const char *service);
int socket_connect(const char *node, const char *service);

struct sockaddr_str { char c[INET6_ADDRSTRLEN]; char zero; };
struct sockaddr_str sockaddr_str(struct sockaddr *addr);
inline struct sockaddr_str sockaddrs_str(struct sockaddr_storage *addr)
{
    return sockaddr_str((struct sockaddr *) addr);
}

// -----------------------------------------------------------------------------
// sigintfd
// -----------------------------------------------------------------------------

int sigintfd_new(void);
bool sigintfd_read(int fd);
void sigintfd_close(int fd);
