/* active.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/prog.h"
#include "game/world.h"
#include "vm/vm.h"

struct chunk;


// -----------------------------------------------------------------------------
// active_config
// -----------------------------------------------------------------------------

typedef void (*make_fn_t) (void *state, id_t id, struct chunk *, uint32_t data);
typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);
typedef void (*load_fn_t) (void *state, struct chunk *);
typedef void (*io_fn_t) (
        void *state, struct chunk *,
        enum atom_io io, id_t src, size_t len, const word_t *args);

struct active_config
{
    size_t size;

    make_fn_t make;
    init_fn_t init;
    load_fn_t load;
    step_fn_t step;
    io_fn_t io;

    size_t io_list_len;
    const word_t *io_list;
};

enum { item_state_len_max = s_cache_line * 4 };

const struct active_config *active_config(enum item);


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

enum legion_packed ports_state
{
    ports_nil = 0,
    ports_requested,
    ports_received,
};

struct legion_packed ports
{
    enum item in, out;
    enum ports_state in_state;
    legion_pad(1);
};

static_assert(sizeof(struct ports) == 4);


// -----------------------------------------------------------------------------
// active
// -----------------------------------------------------------------------------

struct active;

struct active *active_alloc(enum item type);
void active_free(struct active *);

struct active *active_load(enum item type, struct chunk *chunk, struct save *save);
void active_save(struct active *, struct save *save);

size_t active_count(struct active *);

void active_list(struct active *, struct vec64 *ids);
void *active_get(struct active *, id_t id);
struct ports *active_ports(struct active *active, id_t id);

bool active_copy(struct active *, id_t id, void *dst, size_t len);
void active_create(struct active *);
void active_create_from(struct active *, struct chunk *, uint32_t data);
void active_delete(struct active *, id_t id);

void active_step(struct active *, struct chunk *);
bool active_io(struct active *, struct chunk *,
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args);


// -----------------------------------------------------------------------------
// active_list
// -----------------------------------------------------------------------------

typedef struct active *active_list_t[ITEMS_ACTIVE_LEN];

inline struct active *active_index(active_list_t *list, enum item item)
{
    assert(item_is_active(item));
    size_t index =  item - ITEM_ACTIVE_FIRST;

    return (*list)[index];
}

inline struct active *active_index_create(active_list_t *list, enum item item)
{
    assert(item_is_active(item));
    size_t index =  item - ITEM_ACTIVE_FIRST;

    if (unlikely(!(*list)[index])) (*list)[index] = active_alloc(item);
    return (*list)[index];
}

typedef struct active **active_it_t;
inline active_it_t active_next(active_list_t *list, active_it_t it)
{
    const active_it_t end = (*list) + ITEMS_ACTIVE_LEN;
    assert(it < end);

    it = it ? it+1 : *list;
    while (!*it && it < end) it++;
    return likely(it < end) ? it : NULL;
}

void active_list_load(active_list_t *, struct chunk *, struct save *);
void active_list_save(active_list_t *, struct save *);



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
    legion_pad(4);

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
    scanner_wide,
    scanner_target,
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
