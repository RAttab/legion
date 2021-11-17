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
    id_t id;
    enum io io;
    enum ioe err;
};

static_assert(sizeof(struct logi) == 20);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct log;

struct log *log_new(size_t cap);
void log_free(struct log *);

void log_push(struct log *, world_ts_t, struct coord, id_t, enum io, enum ioe);
const struct logi *log_next(const struct log *, const struct logi *it);

void log_save(const struct log *, struct save *);
struct log *log_load(struct save *);

void log_save_delta(const struct log *, struct save *, const struct ack *);
bool log_load_delta(struct log *, struct save *, const struct ack *);
