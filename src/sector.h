/* sector.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "coord.h"
#include "utils.h"

// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------


enum { num_elements = 26 };
typedef uint32_t elements_t [num_elements];


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
    uint16_t planets[];
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
