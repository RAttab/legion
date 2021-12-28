/* net.h
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// socket
// -----------------------------------------------------------------------------

int socket_listen(const char *node, const char *service);
int socket_connect(const char *node, const char *service);


// -----------------------------------------------------------------------------
// sigintfd
// -----------------------------------------------------------------------------

int sigintfd_new(void);
bool sigintfd_read(int fd);
void sigintfd_close(int fd);
