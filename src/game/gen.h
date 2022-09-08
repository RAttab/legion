/* gen.h
   Rémi Attab (remi.attab@gmail.com), 08 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "vm/vm.h"

struct world;
struct atoms;


// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

struct sector *gen_sector(struct coord, seed);
word gen_name_star(struct coord, seed, struct atoms *);
struct symbol gen_name_sector(struct coord, seed);

void gen_populate(struct atoms *);
