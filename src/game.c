/* game.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "db.h"
#include "vm.h"
#include "game.h"
#include "items.h"
#include "ux.h"
#include "engine.h"

#include "utils/rng.h"
#include "utils/vec.h"
#include "utils/str.h"
#include "utils/hset.h"
#include "utils/save.h"
#include "utils/time.h"
#include "utils/config.h"

#include <stdarg.h>
#include <stdatomic.h>
#include <unistd.h>

#include "game/active.h"
#include "game/types.c"
#include "game/coord.c"
#include "game/log.c"
#include "game/lanes.c"
#include "game/sector.c"
#include "game/shards.c"
#include "game/world.c"
#include "game/tape.c"
#include "game/chunk.c"
#include "game/active.c"
#include "game/pills.c"
#include "game/proxy.c"
#include "game/tech.c"
#include "game/energy.c"
#include "game/sim.c"
#include "game/protocol.c"
#include "game/man.c"
