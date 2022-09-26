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

    bits_init(&active->free);
}

void active_free(struct active *active)
{
    free(active->arena);
    free(active->ports);
    bits_free(&active->free);

    bits_init(&active->free);
    active->arena = 0;
    active->ports = 0;
    active->count = 0;
    active->len = 0;
    active->cap = 0;
}

bool active_delete(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len) return false;
    if (bits_test(&active->free, index)) return false;

    bits_set(&active->free, index);
    active->count--;

    if (!active->count && !active->create)
        active_free(active);

    return true;
}

static bool active_recycle(struct active *active, size_t *index)
{
    if (likely(!active->free.len)) return false;

    size_t ix = bits_next(&active->free, 0);
    if (ix == active->free.len) return false;

    bits_unset(&active->free, ix);
    *index = ix;
    return true;
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
    hash = bits_hash(&active->free, hash);

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
    bits_save(&active->free, save);

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

    if (!bits_load(&active->free, save)) return false;

    const struct im_config *config = im_config_assert(active->type);
    if (config->im.load) {
        for (size_t i = 0; i < active->len; ++i) {
            if (bits_test(&active->free, i)) continue;
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
        if (!bits_test(&active->free, ix))
            return make_im_id(active->type, ix);
    }

    return 0;
}

void active_list(struct active *active, struct vec16 *ids)
{
    for (size_t i = 0; i < active->len; ++i) {
        if (bits_test(&active->free, i)) continue;
        vec16_append(ids, make_im_id(active->type, i+1));
    }
}

void *active_get(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len || bits_test(&active->free, index)) return NULL;

    return active->arena + (index * active->size);
}

struct ports *active_ports(struct active *active, im_id id)
{
    size_t index = im_id_seq(id)-1;
    if (index >= active->len || bits_test(&active->free, index)) return NULL;

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

    if (!active->len) {
        active->cap = 1;
        active->arena = calloc(active->cap, active->size);
        active->ports = calloc(active->cap, sizeof(active->ports[0]));
        bits_grow(&active->free, active->cap);
        return;
    }

    active->cap *= 2;
    active->arena = reallocarray(active->arena, active->cap, active->size);
    active->ports = reallocarray(active->ports, active->cap, sizeof(active->ports[0]));

    size_t added = active->cap - active->len;
    memset(active->arena + (active->len * active->size), 0, added * active->size);
    memset(active->ports + active->len, 0, added * sizeof(active->ports[0]));

    bits_grow(&active->free, active->cap);
}

// Given that an item can replicate itself through this function (assembler
// generating a new assembler), moving the pointers around would break things so
// we prefer to defer to creation process.
bool active_create(struct active *active)
{
    if (active->count + active->create == chunk_item_cap) return false;

    active->create++;
    return true;
}

// This creation function is not used for replication (...yet) which means that
// we don't need to defer the creation (... yet)
bool active_create_from(
        struct active *active, struct chunk *chunk,
        const vm_word *data, size_t len)
{
    if (unlikely(active->count + active->create == chunk_item_cap))
        return false;

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
            if (bits_test(&active->free, i)) continue;
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
