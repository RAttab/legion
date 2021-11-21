/* active.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "items/io.h"
#include "items/item.h"
#include "items/config.h"
#include "game/world.h"

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
    legion_pad(5);

    uint16_t count, len, cap;
    uint16_t create;

    void *arena;
    struct ports *ports;
    uint64_t free;

    im_step_t step;
    im_io_t io;
    legion_pad(8);
};

static_assert(sizeof(struct active) == s_cache_line);

void active_init(struct active *, enum item type);
void active_free(struct active *);

bool active_load(struct active *, struct save *, struct chunk *);
void active_save(const struct active *, struct save *save);

size_t active_count(struct active *);

void active_list(struct active *, struct vec64 *ids);
void *active_get(struct active *, id_t id);
struct ports *active_ports(struct active *active, id_t id);

bool active_copy(struct active *, id_t id, void *dst, size_t len);
void active_create(struct active *);
void active_create_from(struct active *, struct chunk *, const word_t *data, size_t len);
void active_delete(struct active *, id_t id);

void active_step(struct active *, struct chunk *);
bool active_io(struct active *, struct chunk *,
        enum io io, id_t src, id_t dst, const word_t *args, size_t len);
