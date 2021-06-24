/* active.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/prog.h"
#include "vm/vm.h"

struct chunk;


// -----------------------------------------------------------------------------
// item_config
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);
typedef void (*io_fn_t) (
        void *state, struct chunk *,
        enum atom_io io, id_t src, size_t len, const word_t *args);

struct item_config
{
    size_t size;
    init_fn_t init;
    step_fn_t step;
    io_fn_t io;
};

const struct item_config *item_config(item_t);

enum { item_state_len_max = s_cache_line * 4 };

// -----------------------------------------------------------------------------
// progable
// -----------------------------------------------------------------------------

enum legion_packed progable_state
{
    progable_nil = 0,
    progable_blocked,
    progable_error,
};

struct legion_packed progable
{
    id_t id;
    uint16_t loops;
    enum progable_state state;

    prog_it_t index;
    const struct prog *prog;
};
static_assert(sizeof(struct progable) == 16);

static const uint16_t progable_loops_inf = UINT16_MAX;


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

enum { brain_msg_cap = 4 };

struct legion_packed brain
{
    id_t id;
    legion_pad(4);

    id_t msg_src;
    uint8_t msg_len;
    legion_pad(3);
    word_t msg[brain_msg_cap];

    legion_pad(8);

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
// worker
// -----------------------------------------------------------------------------

struct legion_packed worker
{
    id_t id;
    id_t src, dst;
    item_t item;
    legion_pad(3);
};

static_assert(sizeof(struct worker) == 16);
