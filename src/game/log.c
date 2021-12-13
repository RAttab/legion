/* log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/log.h"
#include "game/save.h"
#include "game/protocol.h"


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct log
{
    uint32_t it, cap;
    struct logi items[];
};


struct log *log_new(size_t cap)
{
    assert(u64_pop(cap) == 1); // power of two required for overflow

    struct log *log = calloc(1, sizeof(*log) + cap * sizeof(log->items[0]));
    log->cap = cap;
    return log;
}

void log_free(struct log *log)
{
    free(log);
}

void log_push(
        struct log *log,
        world_ts_t time,
        struct coord star,
        id_t id,
        enum io io,
        enum ioe err)
{
    log->items[log->it % log->cap] = (struct logi) {
        .time = time,
        .star = star,
        .id = id,
        .io = io,
        .err = err,
    };
    log->it++;
};

const struct logi *log_next(const struct log *log, const struct logi *it)
{
    if (it == log->items + (log->it % log->cap)) return NULL;
    if (!it) it = log->items + (log->it % log->cap);

    if (it > log->items) it--;
    else it = log->items + (log->cap - 1);

    return it->err ? it : NULL;
}


void log_save(const struct log *log, struct save *save)
{
    save_write_magic(save, save_magic_log);

    save_write_value(save, log->cap);
    save_write_value(save, log->it);
    save_write(save, log->items, log->cap * sizeof(log->items[0]));

    save_write_magic(save, save_magic_log);
}

struct log *log_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_log)) return NULL;

    uint32_t cap = save_read_type(save, typeof(cap));
    struct log *log = log_new(cap);

    save_read_into(save, &log->it);
    save_read(save, log->items, log->cap * sizeof(log->items[0]));

    if (!save_read_magic(save, save_magic_log)) goto fail;
    return log;

  fail:
    log_free(log);
    return NULL;
}

void log_save_delta(const struct log *log, struct save *save, world_ts_t ack)
{
    save_write_magic(save, save_magic_log);

    for (const struct logi *it = log_next(log, NULL);
         it; it = log_next(log, it))
    {
        if (it->time <= ack) continue;
        save_write_value(save, it->time);
        save_write_value(save, coord_to_u64(it->star));
        save_write_value(save, it->id);
        save_write_value(save, it->io);
        save_write_value(save, it->err);
    }
    save_write_value(save, (world_ts_t) 0);

    save_write_magic(save, save_magic_log);
}

bool log_load_delta(struct log *log, struct save *save, world_ts_t ack)
{
    if (!save_read_magic(save, save_magic_log)) return false;

    while (true) {
        world_ts_t time = save_read_type(save, typeof(time));
        if (!time) break;

        struct coord star = coord_from_u64(save_read_type(save, uint64_t));
        id_t id = save_read_type(save, typeof(id));
        enum io io = save_read_type(save, typeof(io));
        enum ioe ioe = save_read_type(save, typeof(ioe));

        if (time <= ack) continue;
        log_push(log, time, star, id, io, ioe);
    }

    return save_read_magic(save, save_magic_log);
}
