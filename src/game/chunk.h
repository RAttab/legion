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
struct chunk *chunk_alloc(
        struct world *, const struct star *, user_id, vm_word name);
void chunk_free(struct chunk *);

void chunk_save(struct chunk *, struct save *);
struct chunk *chunk_load(struct world *, struct save *);

void chunk_save_delta(struct chunk *, struct save *, const struct ack *);
bool chunk_load_delta(struct chunk *, struct save *, struct ack *);

user_id chunk_owner(struct chunk *);
struct world *chunk_world(const struct chunk *);
const struct star *chunk_star(const struct chunk *);
struct tech *chunk_tech(const struct chunk *);
world_ts chunk_updated(const struct chunk *);

vm_word chunk_name(struct chunk *);
void chunk_rename(struct chunk *, vm_word);
bool chunk_harvest(struct chunk *, enum item item);

im_id chunk_last(struct chunk *, enum item);
struct vec16 *chunk_list(struct chunk *);
struct vec16 *chunk_list_filter(struct chunk *, im_list filter);
const void *chunk_get(struct chunk *, im_id);
bool chunk_copy(struct chunk *, im_id, void *dst, size_t len);
bool chunk_delete(struct chunk *, im_id id);
bool chunk_create(struct chunk *, enum item);
bool chunk_create_from(struct chunk *, enum item, const vm_word *data, size_t len);


void chunk_step(struct chunk *);
bool chunk_io(
        struct chunk *,
        enum io io, im_id src, im_id dst,
        const vm_word *args, size_t len);


void chunk_log(struct chunk *, im_id, vm_word key, vm_word value);
const struct log *chunk_logs(struct chunk *);


struct energy *chunk_energy(struct chunk *);


ssize_t chunk_scan(struct chunk *, enum item);


bool chunk_lanes_dock(struct chunk *, vm_word *data);
void chunk_lanes_listen(struct chunk *, im_id, struct coord src, uint8_t chan);
void chunk_lanes_unlisten(struct chunk *, im_id, struct coord src, uint8_t chan);
void chunk_lanes_arrive(
        struct chunk *,
        enum item, struct coord src,
        const vm_word *data, size_t len);
void chunk_lanes_launch(
        struct chunk *,
        enum item item, size_t speed,
        struct coord dst,
        const vm_word *data, size_t len);


void chunk_ports_reset(struct chunk *, im_id);
bool chunk_ports_produce(struct chunk *, im_id, enum item);
bool chunk_ports_consumed(struct chunk *, im_id);
enum item chunk_ports_consume(struct chunk *, im_id);
void chunk_ports_request(struct chunk *, im_id, enum item);


struct workers
{
    uint16_t count, queue, idle, fail, clean;
    struct vec32 *ops;
};

struct workers chunk_workers(struct chunk *);
inline void chunk_workers_ops(uint32_t val, im_id *src, im_id *dst)
{
    *src = val >> 16;
    *dst = val & ((1ULL << 16) - 1);
}
