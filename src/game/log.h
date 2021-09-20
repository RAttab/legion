/* log.h
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/world.h"
#include "items/io.h"
#include "items/item.h"


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
const struct logi *log_next(struct log *, const struct logi *it);

void log_save(const struct log *, struct save *);
struct log *log_load(struct save *);