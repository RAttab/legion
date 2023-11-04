/* game.h
   Remi Attab (remi.attab@gmail.com), 29 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "game/types.h"

#include "vm/vm.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "db/io.h"
#include "db/items.h"
#include "items/config.h"
#include "utils/bits.h"
#include "utils/hash.h"
#include "utils/ring.h"
#include "utils/heap.h"
#include "utils/htable.h"
#include "utils/symbol.h"

#include "game/tape.h"
#include "game/sys.h"
#include "game/user.h"
#include "game/man.h"
#include "game/pills.h"
#include "game/tech.h"
#include "game/chunk.h"

#include "game/world.h"
#include "game/sector.h"
#include "game/energy.h"
#include "game/lanes.h"
#include "game/log.h"

#include "game/protocol.h"
#include "game/sim.h"
#include "game/proxy.h"
