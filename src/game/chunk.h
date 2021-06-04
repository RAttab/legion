/* chunk.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/atoms.h"
#include "vm/vm.h"

struct star;
struct vec64;


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

struct chunk;

struct chunk *chunk_alloc(const struct star *);
void chunk_free(struct chunk *);

struct star *chunk_star(struct chunk *);
bool chunk_harvest(struct chunk *, item_t item);

struct vec64 *chunk_list(struct chunk *);
void *chunk_get(struct chunk *, id_t);
void chunk_create(struct chunk *, item_t);

void chunk_step(struct chunk *);
bool chunk_cmd(struct chunk *, enum atom_io cmd, id_t src, id_t dst, word_t arg);

void chunk_io_reset(struct chunk *, id_t);

bool chunk_io_produce(struct chunk *, id_t, item_t);
void chunk_io_request(struct chunk *, id_t, item_t);
item_t chunk_io_consume(struct chunk *, id_t);

void chunk_io_give(struct chunk *, id_t, item_t);
item_t chunk_io_take(struct chunk *, id_t);
bool chunk_io_pair(struct chunk *, item_t *item, id_t *src, id_t *dst);
