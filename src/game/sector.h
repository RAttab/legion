/* sector.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/coord.h"

// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

legion_packed struct system
{
    struct coord coord;
    uint32_t star;
    uint32_t elements[num_elements];
};

struct system_desc
{
    struct system s;
    size_t planets_len;
    elements_t planets[];
};

legion_packed struct sector
{
    size_t len;
    struct coord coord;

    size_t systems_len;
    struct system systems[];
};


struct sector *sector_gen(struct coord);
struct sector *sector_save(struct sector *, const char *path);

struct sector *sector_load(const char *path);
void sector_close(struct sector *);

struct system *sector_lookup(struct sector *, struct rect *);

struct system_desc *system_gen(struct coord coord);
