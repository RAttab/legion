/* active.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/active.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// active
// -----------------------------------------------------------------------------


void active_init(struct active *active, enum item type)
{
    const struct im_config *config = im_config(type);
    if (!config) { active->skip = true; return; }

    *active = (struct active) {
        .skip = false,
        .type = type,
        .size = config->size,
        .step = config->im.step,
        .io = config->im.io,
    };
}

void active_free(struct active *active)
{
    free(active->arena);
    free(active->ports);
    if (active->cap > 64)
        vec64_free((void *) active->free);

    active->arena = 0;
    active->ports = 0;
    active->free = 0;
    active->count = 0;
    active->free = 0;
    active->len = 0;
    active->cap = 0;
}

bool active_delete(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len) return false;

    if (likely(active->cap <= 64)){
        const uint64_t mask = 1ULL << index;

        if (active->free & mask) return false;
        active->free |= mask;
    }
    else {
        struct vec64 *vec = (void *) active->free;
        const uint64_t mask = 1ULL << (index % 64);
        const size_t ix = index / 64;

        if (vec->vals[ix] & mask) return false;
        vec->vals[ix] |= mask;
    }

    active->count--;
    if (!active->count && !active->create)
        active_free(active);

    return true;
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

hash_val active_hash(const struct active *active, hash_val hash)
{
    if (active->skip) return hash;

    hash = hash_value(hash, active->type);
    hash = hash_value(hash, active->size);
    hash = hash_value(hash, active->count);
    hash = hash_value(hash, active->create);
    hash = hash_bytes(hash, active->arena, active->len * active->size);
    hash = hash_bytes(hash, active->ports, active->len * sizeof(*active->ports));

    if (likely(active->cap <= 64)) hash = hash_value(hash, active->free);
    else {
        struct vec64 *vec = (void *) active->free;
        hash = hash_bytes(hash, vec->vals, vec->len * sizeof(vec->vals[0]));
    }

    return hash;
}

void active_save(const struct active *active, struct save *save)
{
    save_write_magic(save, save_magic_active);

    save_write_value(save, active->len);
    save_write_value(save, active->cap);
    save_write_value(save, active->create);
    if (!active->len && !active->create)
        return save_write_magic(save, save_magic_active);
    save_write_value(save, active->count);

    save_write(save, active->arena, active->len * active->size);
    save_write(save, active->ports, active->len * sizeof(*active->ports));

    if (active->cap < 64) save_write_value(save, active->free);
    else save_write_vec64(save, (const void *) active->free);

    save_write_magic(save, save_magic_active);
}

bool active_load(struct active *active, struct save *save, struct chunk *chunk)
{
    if (!save_read_magic(save, save_magic_active)) return false;

    save_read_into(save, &active->len);
    save_read_into(save, &active->cap);
    save_read_into(save, &active->create);
    if (!active->len && !active->create)
        return save_read_magic(save, save_magic_active);
    save_read_into(save, &active->count);

    active->arena = realloc(active->arena, active->cap * active->size);
    save_read(save, active->arena, active->len * active->size);
    memset(active->arena + (active->len * active->size), 0,
            (active->cap - active->len) * active->size);

    active->ports = realloc(active->ports, active->cap * sizeof(*active->ports));
    save_read(save, active->ports, active->len * sizeof(*active->ports));
    memset(active->ports + active->len, 0,
            (active->cap - active->len) * sizeof(*active->ports));

    if (active->cap < 64) save_read_into(save, &active->free);
    else if (!save_read_vec64(save, (struct vec64 **) &active->free))
        return false;

    const struct im_config *config = im_config_assert(active->type);
    if (config->im.load) {
        for (size_t i = 0; i < active->len; ++i) {
            if (active_deleted(active, i)) continue;
            if (chunk) config->im.load(active->arena + (i * active->size), chunk);
        }
    }

    return save_read_magic(save, save_magic_active);
}

size_t active_count(struct active *active)
{
    return active->count;
}


im_id active_last(struct active *active)
{
    for (size_t i = 0; i < active->len; ++i) {
        size_t ix = active->len - i - 1;
        if (!active_deleted(active, ix))
            return make_im_id(active->type, ix);
    }

    return 0;
}

void active_list(struct active *active, struct vec64 *ids)
{
    for (size_t i = 0; i < active->len; ++i) {
        if (active_deleted(active, i)) continue;
        vec64_append(ids, make_im_id(active->type, i+1));
    }
}

void *active_get(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len || active_deleted(active, index)) return NULL;

    return active->arena + (index * active->size);
}

struct ports *active_ports(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len || active_deleted(active, index)) return NULL;

    return &active->ports[index];
}

bool active_copy(struct active *active, im_id id, void *dst, size_t len)
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
bool active_create(struct active *active)
{
    if (active->count + active->create == active_cap) return false;

    active->create++;
    return true;
}

// This creation function is not used for replication (...yet) which means that
// we don't need to defer the creation (... yet)
bool active_create_from(
        struct active *active, struct chunk *chunk, const vm_word *data, size_t len)
{
    if (active->count + active->create == active_cap) return false;

    const struct im_config *config = im_config_assert(active->type);
    assert(config->im.make);

    size_t index = 0;
    if (!active_recycle(active, &index)) {
        active_grow(active);
        index = active->len;
        active->len++;
    }

    im_id id = make_im_id(active->type, index+1);
    void *item = active->arena + (index * active->size);

    config->im.make(item, chunk, id, data, len);
    active->count++;

    return true;
}

void active_step(
        struct active *active, struct chunk *chunk)
{
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

        im_id id = make_im_id(active->type, index+1);
        void *item = active->arena + (index * active->size);
        const struct im_config *config = im_config_assert(active->type);

        config->im.init(item, chunk, id);
        active->create--;
        active->count++;
    }
}

bool active_io(
        struct active *active, struct chunk *chunk,
        enum io io, im_id src, im_id dst, const vm_word *args, size_t len)
{
    void *state = active_get(active, dst);
    if (!state) return false;

    active->io(state, chunk, io, src, args, len);
    return true;
}
