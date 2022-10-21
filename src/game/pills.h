/* pills.h
   Rémi Attab (remi.attab@gmail.com), 14 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "db/items.h"
#include "items/types.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// pills
// -----------------------------------------------------------------------------

struct pills
{
    uint16_t count, cap;

    struct bits free, match;
    struct coord *coord;
    struct cargo *cargo;
};

void pills_init(struct pills *);
void pills_free(struct pills *);

hash_val pills_hash(struct pills *, hash_val);
bool pills_load(struct pills *, struct save *);
void pills_save(struct pills *, struct save *);

size_t pills_count(struct pills *);

struct pills_ret
{
    bool ok;
    struct coord coord;
    struct cargo cargo;
};

struct pills_ret pills_dock(struct pills *, struct coord, enum item);
bool pills_arrive(struct pills *, struct coord, struct cargo);
