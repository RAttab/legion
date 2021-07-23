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
