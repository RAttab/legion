/* log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/log.h"


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void log_push(
        struct log *log,
        world_ts_t time,
        struct coord star,
        id_t id,
        enum io io,
        enum ioe err)
{
    log->items[log->it % log_cap] = (struct logi) {
        .time = time,
        .star = star,
        .id = id,
        .io = io,
        .err = err,
    };
    log->it++;
};

const struct logi *log_next(struct log *log, const struct logi *it)
{
    if (it == log->items + (log->it % log_cap)) return NULL;
    if (!it) it = log->items + (log->it % log_cap);

    if (it > log->items) it--;
    else it = log->items + (log_cap - 1);

    return it->err ? it : NULL;
}


void log_save(const struct log *log, struct save *save)
{
    save_write_magic(save, save_magic_log);

    save_write_value(save, log->it);
    save_write(save, log->items, sizeof(log->items));

    save_write_magic(save, save_magic_log);
}

bool log_load(struct log *log, struct save *save)
{
    if (!save_read_magic(save, save_magic_log)) return false;

    save_read_into(save, &log->it);
    save_read(save, log->items, sizeof(log->items));

    return save_read_magic(save, save_magic_log);
}
