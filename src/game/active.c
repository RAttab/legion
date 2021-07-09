/* active.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "active.h"

// -----------------------------------------------------------------------------
// active_list
// -----------------------------------------------------------------------------

void active_list_load(active_list_t *list, struct chunk *chunk, struct save *save)
{
    for (size_t i = 0; i < array_len(*list); ++i)
        (*list)[i] = active_load(i + ITEM_ACTIVE_FIRST, chunk, save);
}

void active_list_save(active_list_t *list, struct save *save)
{
    for (size_t i = 0; i < array_len(*list); ++i)
        active_save((*list)[i], save);
}

// -----------------------------------------------------------------------------
// active
// -----------------------------------------------------------------------------

legion_packed struct active
{
    enum item type;
    uint8_t size;
    uint16_t create;
    legion_pad(4);

    uint16_t count, len, cap;
    legion_pad(2);

    void *arena;
    struct ports *ports;
    uint64_t free;

    step_fn_t step;
    io_fn_t io;
    const struct active_config *config;
};

static_assert(sizeof(struct active) == s_cache_line);


struct active *active_alloc(enum item type)
{
    const struct active_config *config = active_config(type);

    struct active *active = calloc(1, sizeof(*active));
    *active = (struct active) {
        .type = type,
        .size = config->size,
        .step = config->step,
        .io = config->io,
        .config = config,
    };

    return active;
}

void active_free(struct active *active)
{
    if (!active) return;
    free(active->arena);
    free(active->ports);
}

void active_delete(struct active *active, id_t id)
{
    if (!active) return;

    size_t index = id_bot(id)-1;
    if (index >= active->len) return;

    if (likely(active->cap <= 64))
        active->free &= 1ULL << index;
    else {
        struct vec64 *vec = (void *) active->free;
        vec->vals[index / 64] &= 1ULL << (index % 64);
    }
}

inline bool active_deleted(struct active *active, size_t index)
{
    if (likely(!active->free)) return false;
    if (likely(active->cap <= 64)) return (active->free & (1ULL << index)) != 0;

    struct vec64 *vec = (void *) active->free;
    return (vec->vals[index / 64] & (1ULL << (index % 64))) != 0;
}

static bool active_recycle(struct active *active, size_t *index)
{
    if (likely(!active->free)) return false;

    if (likely(active->cap <= 64)) {
        *index = u64_ctz(active->free);
        active->free &= ~(1ULL << *index);
        return true;
    }

    struct vec64 *vec = (void *) active->free;
    for (size_t i = 0; i < vec->len; ++i) {
        if (!vec->vals[i]) continue;

        *index = u64_ctz(vec->vals[i]);
        vec->vals[i] &= ~(1ULL << *index);
        *index += (i * 64);
        return true;
    }

    return false;
}


struct active *active_load(enum item type, struct chunk *chunk, struct save *save)
{
    uint16_t len = save_read_type(save, typeof(len));
    if (!len) return NULL;

    struct active *active = active_alloc(type);
    active->len = len;
    save_read_into(save, &active->count);

    active->cap = 1;
    while (active->cap < len) active->cap *= 2;

    active->arena = calloc(active->cap, active->size);
    save_read(save, active->arena, active->cap * active->size);

    active->ports = calloc(active->cap, sizeof(*active->ports));
    save_read(save, active->ports, active->cap * sizeof(*active->ports));

    if (active->cap < 64) save_read_into(save, &active->free);
    else active->free = (uintptr_t) save_read_vec64(save);

    for (size_t i = 0; i < len; ++i) {
        if (!active->config->load || active_deleted(active, i)) continue;
        active->config->load(active->arena + (i * active->size), chunk);
    }

    return active;
}

void active_save(struct active *active, struct save *save)
{
    uint16_t len = active ? active->len : 0;
    save_write_value(save, len);
    if (!active) return;

    // would mean that we're saving mid step and that's a bad idea.
    assert(!active->create);

    save_write_value(save, active->count);
    save_write(save, active->arena, len * active->size);
    save_write(save, active->ports, len * sizeof(struct ports));

    if (active->cap < 64) save_write_value(save, active->free);
    else save_write_vec64(save, (const void *) active->free);
}


size_t active_count(struct active *active)
{
    return active ? active->count : 0;
}

void active_list(struct active *active, struct vec64 *ids)
{
    if (!active) return;

    for (size_t i = 0; i < active->len; ++i) {
        if (active_deleted(active, i)) continue;
        vec64_append(ids, make_id(active->type, i+1));
    }
}

void *active_get(struct active *active, id_t id)
{
    if (!active) return NULL;

    size_t index = id_bot(id)-1;
    if (index >= active->len || active_deleted(active, index)) return NULL;

    return active->arena + (index * active->size);
}

struct ports *active_ports(struct active *active, id_t id)
{
    if (!active) return NULL;

    size_t index = id_bot(id)-1;
    if (index >= active->len || active_deleted(active, index)) return NULL;

    return &active->ports[index];
}

bool active_copy(struct active *active, id_t id, void *dst, size_t len)
{
    assert(len >= active->size);

    void *src = active_get(active, id);
    if (!src) return false;

    memcpy(dst, src, active->size);
    return true;
}


// \todo calloc and reallocarray won't cache align anything so gotta do it
// manually.
static void active_grow(struct active *active)
{
    if (likely(active->len < active->cap)) return;
    size_t old = active->cap;

    if (!active->len) {
        active->cap = 1;
        active->arena = calloc(active->cap, active->size);
        active->ports = calloc(active->cap, sizeof(active->ports[0]));
        return;
    }

    active->cap *= 2;
    active->arena = reallocarray(active->arena, active->cap, active->size);
    active->ports = reallocarray(active->ports, active->cap, sizeof(active->ports[0]));

    size_t added = active->cap - active->len;
    memset(active->arena + (active->len * active->size), 0, added * active->size);
    memset(active->ports + active->len, 0, added * sizeof(active->ports[0]));

    if (old > 64)
        active->free = (uintptr_t) vec64_grow((void *) active->free, active->cap / 64);
    else if (old == 64) {
        struct vec64 *vec = vec64_reserve(2);
        vec->vals[0] = active->free;
        active->free = (uintptr_t) vec;
    }
}

// Given that an item can replicate itself through this function (assembler
// generating a new assembler), moving the pointers around would break things so
// we prefer to defer to creation process.
void active_create(struct active *active)
{
    active->create++;
}

// This creation function is not used for replication (...yet) which means that
// we don't need to defer the creation (... yet)
void active_create_from(struct active *active, struct chunk *chunk, uint32_t data)
{
    assert(active->config->make);

    size_t index = 0;
    if (!active_recycle(active, &index)) {
        active_grow(active);
        index = active->len;
        active->len++;
    }

    id_t id = make_id(active->type, index+1);
    void *item = active->arena + (index * active->size);
    active->config->make(item, id, chunk, data);
    active->count++;
}

void active_step(struct active *active, struct chunk *chunk)
{
    if (!active) return;

    if (active->step) {
        for (size_t i = 0; i < active->len; ++i) {
            if (active_deleted(active, i)) continue;
            active->step(active->arena + (i * active->size), chunk);
        }
    }

    while (active->create) {
        size_t index = 0;
        if (!active_recycle(active, &index)) {
            active_grow(active);
            index = active->len;
            active->len++;
        }

        id_t id = make_id(active->type, index+1);
        void *item = active->arena + (index * active->size);

        active->config->init(item, id, chunk);
        active->create--;
        active->count++;
    }
}

bool active_io(
        struct active *active, struct chunk *chunk,
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args)
{
    void *state = active_get(active, dst);
    if (!state) return false;

    active->io(state, chunk, io, src, len, args);
    return true;
}



// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

#include "game/item/brain.c"
#include "game/item/db.c"
#include "game/item/deploy.c"
#include "game/item/extract.c"
#include "game/item/legion.c"
#include "game/item/printer.c"
#include "game/item/storage.c"


const struct active_config *active_config(enum item item)
{
    switch (item)
    {
    case ITEM_DEPLOY:                       return deploy_config(item);
    case ITEM_EXTRACT_1...ITEM_EXTRACT_3:   return extract_config(item);
    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3:  return printer_config(item);
    case ITEM_STORAGE:                      return storage_config(item);
    case ITEM_DB_1...ITEM_DB_3:             return db_config(item);
    case ITEM_BRAIN_1...ITEM_BRAIN_3:       return brain_config(item);
    case ITEM_LEGION_1...ITEM_LEGION_3:     return legion_config(item);
    default: { assert(false); }
    }
}
