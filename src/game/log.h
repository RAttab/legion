/* log.h
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/world.h"
#include "items/io.h"
#include "items/item.h"

struct ack;


// -----------------------------------------------------------------------------
// logi
// -----------------------------------------------------------------------------

struct legion_packed logi
{
    struct coord star;
    world_ts_t time;
    id id;
    legion_pad(2);
    word_t key, value;
};

static_assert(sizeof(struct logi) == 32);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct log;

struct log *log_new(size_t cap);
void log_free(struct log *);

void log_push(struct log *, world_ts_t, struct coord, id, word_t key, word_t value);
const struct logi *log_next(const struct log *, const struct logi *it);

void log_save(const struct log *, struct save *);
struct log *log_load(struct save *);

void log_save_delta(const struct log *, struct save *, world_ts_t);
bool log_load_delta(struct log *, struct save *, world_ts_t);
