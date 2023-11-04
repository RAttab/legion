/* db.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/db.h"

#include "vm/vm.h"
#include "game/game.h"
#include "items/config.h"
#include "items/items.h"
#include "utils/err.h"
#include "utils/rng.h"
#include "utils/bits.h"

#include <sys/mman.h>

#include "db/io.c"
#include "db/items.c"
#include "db/tapes.c"
#include "db/specs.c"
#include "db/stars.c"
#include "db/man.c"
