/* io.h
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/atoms.h"

struct atoms;

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

enum legion_packed io
{
    io_min = atom_ns_io,

    #include "gen/io_enum.h"

    io_max,
    io_len = io_max - io_min,
};
static_assert(sizeof(enum io) == 4);

enum legion_packed ioe
{
    ioe_min = atom_ns_ioe,

    #include "gen/ioe_enum.h"

    ioe_max,
    ioe_len = ioe_max - ioe_min,
};
static_assert(sizeof(enum ioe) == 4);


void io_populate_atoms(struct atoms *);

// -----------------------------------------------------------------------------
// io_spec
// -----------------------------------------------------------------------------

struct io_param { const char *name; bool required; };
struct io_cmd { enum io io; size_t len; struct io_param params[8]; };
