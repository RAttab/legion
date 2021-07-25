/* items.h
   Rémi Attab (remi.attab@gmail.com), 23 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint16_t loops_t;
enum { loops_inf = UINT16_MAX };
inline loops_t loops_io(word_t loops)
{
    return loops > 0 && loops < loops_inf ? loops : loops_inf;
}


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

struct legion_packed deploy
{
    id_t id;
    loops_t loops;
    enum item item;
    bool waiting;
};

static_assert(sizeof(struct deploy) == 8);


// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

struct legion_packed extract
{
    id_t id;
    loops_t loops;
    bool waiting;
    legion_pad(1);
    prog_packed_t prog;
};

static_assert(sizeof(struct extract) == 16);


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------
// Also used for assembler. It's the exact same logic.

struct legion_packed printer
{
    id_t id;
    loops_t loops;
    bool waiting;
    legion_pad(1);
    prog_packed_t prog;
};

static_assert(sizeof(struct printer) == 16);


// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

struct legion_packed storage
{
    id_t id;
    enum item item;
    bool waiting;
    uint16_t count;
};

static_assert(sizeof(struct storage) == 8);


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

enum { brain_msg_cap = 4 };

struct legion_packed brain
{
    id_t id;
    uint8_t debug;
    legion_pad(3);

    id_t msg_src;
    uint8_t msg_len;
    legion_pad(3);
    word_t msg[brain_msg_cap];

    legion_pad(4);

    mod_t mod_id;
    const struct mod *mod;

    struct vm vm;
};
static_assert(sizeof(struct brain) == s_cache_line + sizeof(struct vm));


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

struct legion_packed db
{
    id_t id;
    uint8_t len;
    legion_pad(3);
    word_t data[];
};

static_assert(sizeof(struct db) == 8);


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

struct legion_packed legion
{
    id_t id;
    mod_t mod;
};

const enum item *legion_cargo(enum item type);

static_assert(sizeof(struct legion) == 8);


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

enum legion_packed scanner_state
{
    scanner_idle = 0,
    scanner_wide = 1,
    scanner_target = 2,
};

struct legion_packed scanner
{
    id_t id;

    enum scanner_state state;
    struct { uint8_t left; uint8_t cap; } work;
    legion_pad(1);

    union
    {
        struct world_scan_it wide;
        struct { enum item item; struct coord coord; } target;
    } type;

    word_t result;
};

static_assert(sizeof(struct scanner) == 32);
