/* chunk.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct ack;
struct star;
struct logi;
struct vec32;
struct vec64;
struct coord;
struct energy;
struct world;


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

enum : size_t { chunk_item_cap = UINT8_MAX };

struct chunk;


// -----------------------------------------------------------------------------
// init
// -----------------------------------------------------------------------------

struct chunk *chunk_alloc_empty(void);
struct chunk *chunk_alloc(const struct star *, user_id, vm_word name);
void chunk_free(struct chunk *);

void chunk_shard(struct chunk *, struct shard *);


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

void chunk_save(struct chunk *, struct save *);
struct chunk *chunk_load(struct save *, struct shard *);

void chunk_save_delta(struct chunk *, struct save *, const struct ack *);
bool chunk_load_delta(struct chunk *, struct save *, struct ack *);


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

user_id chunk_owner(const struct chunk *);
world_ts chunk_time(const struct chunk *);
world_ts chunk_updated(const struct chunk *);
const struct star *chunk_star(const struct chunk *);
struct energy *chunk_energy(struct chunk *);
const struct mods *chunk_mods(struct chunk *);

const struct tech *chunk_tech(const struct chunk *);
void chunk_tech_learn_bit(const struct chunk *, enum item, uint8_t bit);

vm_word chunk_name(struct chunk *);
void chunk_rename(struct chunk *, vm_word);


// -----------------------------------------------------------------------------
// items
// -----------------------------------------------------------------------------

bool chunk_extract(struct chunk *, enum item);

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


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void chunk_log(struct chunk *, im_id, vm_word key, vm_word value);
const struct log *chunk_logs(struct chunk *);


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

ssize_t chunk_count(struct chunk *, enum item);

void chunk_probe(struct chunk *, struct coord, enum item);
ssize_t chunk_probe_value(struct chunk *, struct coord, enum item);

void chunk_scan(struct chunk *, struct scan_it);
struct coord chunk_scan_value(struct chunk *, struct scan_it);


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct pills *chunk_pills(struct chunk *);
struct pills_ret chunk_pills_dock(struct chunk *, struct coord, enum item);
bool chunk_pills_undock(struct chunk *, struct coord, struct cargo);

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


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

struct workers
{
    uint16_t queue;
    uint8_t count, idle, fail, clean;
    struct vec32 *ops;
};

struct workers chunk_workers(struct chunk *);
inline void chunk_workers_ops(uint32_t val, im_id *src, im_id *dst)
{
    *src = val >> 16;
    *dst = val & ((1ULL << 16) - 1);
}

void chunk_ports_reset(struct chunk *, im_id);
bool chunk_ports_produce(struct chunk *, im_id, enum item);
bool chunk_ports_consumed(struct chunk *, im_id);
enum item chunk_ports_consume(struct chunk *, im_id);
void chunk_ports_request(struct chunk *, im_id, enum item);
