/* active.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "db/io.h"
#include "db/items.h"
#include "items/config.h"
#include "game/world.h"
#include "utils/hash.h"

struct chunk;
struct energy;


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

legion_packed struct active
{
    bool skip;
    enum item type;
    uint8_t size;
    legion_pad(1);

    uint8_t count, len, cap;
    uint8_t create;

    void *arena;
    struct ports *ports;
    struct bits free;

    im_step_fn step;
    im_io_fn io;

    legion_pad(8);
};

static_assert(sizeof(struct active) == s_cache_line);

void active_init(struct active *, enum item type);
void active_free(struct active *);

hash_val active_hash(const struct active *, hash_val hash);
bool active_load(struct active *, struct save *, struct chunk *);
void active_save(const struct active *, struct save *save);

size_t active_count(struct active *);

im_id active_last(struct active *);
void active_list(struct active *, struct vec16 *ids);
void *active_get(struct active *, im_id id);
struct ports *active_ports(struct active *active, im_id id);

bool active_copy(struct active *, im_id id, void *dst, size_t len);
bool active_create(struct active *);
bool active_create_from(
        struct active *, struct chunk *, const vm_word *data, size_t len);
bool active_delete(struct active *, im_id id);

void active_step(struct active *, struct chunk *);
bool active_io(struct active *, struct chunk *,
        enum io io, im_id src, im_id dst, const vm_word *args, size_t len);
