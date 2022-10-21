/* io.h
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct atoms;

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

enum legion_packed io
{
    // We place the IO namespace right after the item namespace
    io_min = 1 << 8,

    #include "gen/io_enum.h"

    io_max,
    io_len = io_max - io_min,
};
static_assert(sizeof(enum io) == 2);

enum legion_packed ioe
{
    // We place the IOE namespace right after the item namespace
    ioe_min = 2 << 8,

    #include "gen/ioe_enum.h"

    ioe_max,
    ioe_len = ioe_max - ioe_min,
};
static_assert(sizeof(enum ioe) == 2);


void io_populate_atoms(struct atoms *);

// -----------------------------------------------------------------------------
// io_spec
// -----------------------------------------------------------------------------

struct io_param { const char *name; bool required; };
struct io_cmd { enum io io; size_t len; struct io_param params[8]; };
