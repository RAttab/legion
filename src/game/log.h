/* log.h
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/world.h"
#include "items/io.h"
#include "db/items.h"

struct ack;


// -----------------------------------------------------------------------------
// logi
// -----------------------------------------------------------------------------

struct legion_packed logi
{
    struct coord star;
    world_ts time;
    im_id id;
    legion_pad(2);
    vm_word key, value;
};

static_assert(sizeof(struct logi) == 32);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct log;

struct log *log_new(size_t cap);
void log_free(struct log *);

void log_push(struct log *, world_ts, struct coord, im_id, vm_word key, vm_word value);
const struct logi *log_next(const struct log *, const struct logi *it);

void log_save(const struct log *, struct save *);
struct log *log_load(struct save *);

void log_save_delta(const struct log *, struct save *, world_ts);
bool log_load_delta(struct log *, struct save *, world_ts);
