/* render.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ux/ux.h"
#include "vm/mod.h"
#include "vm/code.h"
#include "vm/atoms.h"
#include "db/tapes.h"
#include "db/items.h"
#include "db/specs.h"
#include "items/config.h"
#include "items/items.h"
#include "game/game.h"
#include "utils/fs.h"
#include "utils/str.h"
#include "utils/color.h"
#include "utils/htable.h"
#include "utils/vec.h"
#include "utils/hset.h"

#include <stdarg.h>

#include "ux/ux.c"
#include "ux/ux_map.c"
#include "ux/ux_factory.c"
#include "ux/ux_topbar.c"
#include "ux/ux_status.c"
#include "ux/ux_tapes.c"
#include "ux/ux_mods.c"
#include "ux/ux_stars.c"
#include "ux/ux_star.c"
#include "ux/ux_log.c"
#include "ux/ux_item.c"
#include "ux/ux_io.c"
#include "ux/ux_pills.c"
#include "ux/ux_energy.c"
#include "ux/ux_workers.c"
#include "ux/ux_man.c"

