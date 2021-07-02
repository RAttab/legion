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

const struct item_config *item_config(item_t);

bool item_is_progable(item_t);
bool item_is_brain(item_t);
bool item_is_db(item_t);


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
    uint64_t prog;
};
static_assert(sizeof(struct progable) == 16);

static const uint16_t progable_loops_inf = UINT16_MAX;

const struct prog *progable_prog(struct progable *);


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
