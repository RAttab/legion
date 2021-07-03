/* chunk.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/save.h"
#include "game/item.h"
#include "game/atoms.h"
#include "vm/vm.h"

struct star;
struct vec64;
struct world;


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

struct chunk;

struct chunk *chunk_alloc(struct world *, const struct star *);
void chunk_free(struct chunk *);

struct chunk *chunk_load(struct world *, struct save *);
void chunk_save(struct chunk *, struct save *);

struct world *chunk_world(struct chunk *);
struct star *chunk_star(struct chunk *);
bool chunk_harvest(struct chunk *, item_t item);

struct workers { uint16_t count, idle, fail; };
struct workers chunk_workers(struct chunk *);

struct vec64 *chunk_list(struct chunk *);
struct vec64 *chunk_list_filter(struct chunk *, const item_t *filter, size_t len);
void *chunk_get(struct chunk *, id_t);
bool chunk_copy(struct chunk *, id_t, void *dst, size_t len);
void chunk_create(struct chunk *, item_t);

void chunk_step(struct chunk *);
bool chunk_io(
        struct chunk *,
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args);

ssize_t chunk_scan(struct chunk *, item_t);

void chunk_lanes_arrive(struct chunk *, item_t type, item_t cargo, uint8_t count);

void chunk_ports_reset(struct chunk *, id_t);
bool chunk_ports_produce(struct chunk *, id_t, item_t);
void chunk_ports_request(struct chunk *, id_t, item_t);
item_t chunk_ports_consume(struct chunk *, id_t);
