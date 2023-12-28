/* shards.h
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// shard
// -----------------------------------------------------------------------------

struct shard;

// Only used for testing
struct shard *shard_alloc(struct world *);
void shard_free(struct shard *);
void shard_step(struct shard *);
struct chunk *shard_chunk_alloc(
        struct shard *, const struct star *, user_id, vm_word name);

world_ts shard_time(const struct shard *);
const struct mods *shard_mods(struct shard *);
const struct tech *shard_tech(struct shard *, user_id);


void shard_user_io_push(struct shard *, user_id, struct user_io);
void shard_log_push(struct shard *, user_id, struct log_line);
void shard_tech_push(struct shard *, user_id, enum item, uint8_t bit);
void shard_lanes_push(struct shard *, struct lanes_packet);

void shard_probe_push(struct shard *, struct coord src, struct coord dst, enum item);
ssize_t shard_probe_get(const struct shard *, struct coord, enum item);

void shard_scan_push(struct shard *, struct coord src, struct scan_it);
struct coord shard_scan_get(struct shard *, struct scan_it);


// -----------------------------------------------------------------------------
// shards
// -----------------------------------------------------------------------------

struct shards;

struct shards *shards_alloc(struct world *);
void shards_free(struct shards *);

struct shard *shards_get(struct shards *, struct coord);
void shards_register(struct shards *, struct chunk *);

void shards_step(struct shards *);

void shards_save(struct shards *, struct save *);
struct shards *shards_load(struct world *, struct save *);
