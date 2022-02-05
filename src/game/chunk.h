/* chunk.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/save.h"
#include "game/world.h"
#include "items/io.h"
#include "items/item.h"
#include "vm/vm.h"
#include "utils/ring.h"
#include "utils/hash.h"

struct ack;
struct star;
struct logi;
struct vec64;
struct coord;
struct energy;


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

struct chunk;

struct chunk *chunk_alloc_empty(void);
struct chunk *chunk_alloc(struct world *, const struct star *, user_t, word_t name);
void chunk_free(struct chunk *);

void chunk_save(struct chunk *, struct save *);
struct chunk *chunk_load(struct world *, struct save *);

void chunk_save_delta(struct chunk *, struct save *, const struct ack *);
bool chunk_load_delta(struct chunk *, struct save *, struct ack *);

user_t chunk_owner(struct chunk *);
struct world *chunk_world(const struct chunk *);
const struct star *chunk_star(const struct chunk *);
struct tech *chunk_tech(const struct chunk *);
world_ts_t chunk_updated(const struct chunk *);

word_t chunk_name(struct chunk *);
void chunk_rename(struct chunk *, word_t);
bool chunk_harvest(struct chunk *, enum item item);

struct vec64 *chunk_list(struct chunk *);
struct vec64 *chunk_list_filter(struct chunk *, im_list_t filter);
const void *chunk_get(struct chunk *, id_t);
bool chunk_copy(struct chunk *, id_t, void *dst, size_t len);
void chunk_delete(struct chunk *, id_t id);
void chunk_create(struct chunk *, enum item);
void chunk_create_from(struct chunk *, enum item, const word_t *data, size_t len);


void chunk_step(struct chunk *);
bool chunk_io(
        struct chunk *,
        enum io io, id_t src, id_t dst,
        const word_t *args, size_t len);


void chunk_log(struct chunk *, id_t, word_t key, word_t value);
const struct log *chunk_logs(struct chunk *);


struct energy *chunk_energy(struct chunk *);


ssize_t chunk_scan(struct chunk *, enum item);


bool chunk_lanes_dock(struct chunk *, word_t *data);
void chunk_lanes_listen(struct chunk *, id_t, struct coord src, uint8_t chan);
void chunk_lanes_unlisten(struct chunk *, id_t, struct coord src, uint8_t chan);
void chunk_lanes_arrive(
        struct chunk *,
        enum item, struct coord src,
        const word_t *data, size_t len);
void chunk_lanes_launch(
        struct chunk *,
        enum item item, size_t speed,
        struct coord dst,
        const word_t *data, size_t len);


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
