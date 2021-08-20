/* chunk.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/save.h"
#include "items/io.h"
#include "items/item.h"
#include "vm/vm.h"

struct star;
struct vec64;
struct world;
struct coord;


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
bool chunk_harvest(struct chunk *, enum item item);

struct vec64 *chunk_list(struct chunk *);
struct vec64 *chunk_list_filter(struct chunk *, const enum item *filter, size_t len);
const void *chunk_get(struct chunk *, id_t);
bool chunk_copy(struct chunk *, id_t, void *dst, size_t len);
void chunk_delete(struct chunk *, id_t id);
void chunk_create(struct chunk *, enum item);
void chunk_create_from(struct chunk *, enum item, const word_t *data, size_t len);

void chunk_step(struct chunk *);
bool chunk_io(
        struct chunk *, enum io io, id_t src, id_t dst, const word_t *args, size_t len);

uint64_t chunk_known(struct chunk *, enum item);
uint64_t chunk_known_bits(struct chunk *, enum item);
struct tape_set chunk_known_list(struct chunk *);
enum item chunk_learn(struct chunk *, uint64_t hash);
void chunk_learn_bit(struct chunk *, enum item, uint64_t bit);

ssize_t chunk_scan(struct chunk *, enum item);

void chunk_lanes_launch(
        struct chunk *, enum item item, struct coord dst, const word_t *data, size_t len);
bool chunk_lanes_dock(struct chunk *, enum item *item, uint8_t *count);
void chunk_lanes_arrive(struct chunk *, enum item, const word_t *data, size_t len);

void chunk_ports_reset(struct chunk *, id_t);
bool chunk_ports_produce(struct chunk *, id_t, enum item);
bool chunk_ports_consumed(struct chunk *, id_t);
void chunk_ports_request(struct chunk *, id_t, enum item);
enum item chunk_ports_consume(struct chunk *, id_t);

struct workers
{
    uint16_t count, queue, idle, fail, clean;
    struct vec64 *ops;
};

struct workers chunk_workers(struct chunk *);
inline void chunk_workers_ops(uint64_t val, id_t *src, id_t *dst)
{
    *src = val >> 32;
    *dst = val & ((1ULL << 32) - 1);
}
