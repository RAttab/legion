/* legion.c
   Rémi Attab (remi.attab@gmail.com), 02 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game.h"
#include "engine.h"
#include "utils/fs.h"
#include "utils/net.h"
#include "utils/time.h"
#include "utils/symbol.h"
#include "utils/config.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static_assert(EAGAIN == EWOULDBLOCK);

void usage(int code, const char *msg);

#include "legion/local.c"
#include "legion/client.c"
#include "legion/server.c"
#include "legion/config.c"

