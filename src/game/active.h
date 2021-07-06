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
typedef void (*load_fn_t) (void *state, struct chunk *);
typedef void (*io_fn_t) (
        void *state, struct chunk *,
        enum atom_io io, id_t src, size_t len, const word_t *args);

struct item_config
{
    size_t size;

    init_fn_t init;
    step_fn_t step;
    load_fn_t load;
    io_fn_t io;

    size_t io_list_len;
    const word_t *io_list;
};

enum { item_state_len_max = s_cache_line * 4 };

const struct item_config *item_config(enum item);

bool item_is_progable(enum item);
bool item_is_brain(enum item);
bool item_is_db(enum item);


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint16_t loops_t;
static const uint16_t loops_inf = UINT16_MAX;
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
    legion_pad(4);

    id_t msg_src;
    uint8_t msg_len;
    legion_pad(3);
    word_t msg[brain_msg_cap];

    legion_pad(4);

    mod_t mod_id;
    struct mod *mod;

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
    bool arrived;
};

static_assert(sizeof(struct legion) == 8);

static const struct coord legion_deployed = { .x = 0, .y = 0 };
static const struct coord legion_waiting = { .x = UINT32_MAX, .y = UINT32_MAX };
