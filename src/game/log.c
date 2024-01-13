/* log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct log
{
    uint32_t it, cap;
    struct log_line items[];
};


struct log *log_new(size_t cap)
{
    assert(u64_pop(cap) == 1); // power of two required for overflow

    struct log *log = mem_struct_alloc_t(log, log->items[0], cap);
    log->cap = cap;
    return log;
}

void log_free(struct log *log)
{
    mem_free(log);
}

static void log_reset(struct log *log)
{
    size_t cap = log->cap;
    memset(log, 0, sizeof(*log) + cap * sizeof(log->items[0]));
    log->cap = cap;
}

void log_push(struct log *log, struct log_line line)
{
    log->items[log->it % log->cap] = line;
    log->it++;
};

const struct log_line *log_next(const struct log *log, const struct log_line *it)
{
    if (it == log->items + (log->it % log->cap)) return NULL;
    if (!it) it = log->items + (log->it % log->cap);

    if (it > log->items) it--; // walk the log backwards
    else it = log->items + (log->cap - 1);

    return it->id ? it : NULL;
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

// \todo would be nice to push the items in reverse order so we can avoid the
// recursion on load but right now that means doing a bunch of scanning and
// checks to figure out where the bottom is which is an annoying amount of code
// to write.
void log_save_delta(const struct log *log, struct save *save, world_ts ack)
{
    save_write_magic(save, save_magic_log);

    for (const struct log_line *it = log_next(log, NULL);
         it; it = log_next(log, it))
    {
        if (it->time <= ack) continue;
        save_write_value(save, it->time);
        save_write_value(save, coord_to_u64(it->star));
        save_write_value(save, it->id);
        save_write_value(save, it->key);
        save_write_value(save, it->value);
    }
    save_write_value(save, (world_ts) 0);

    save_write_magic(save, save_magic_log);
}

// We need to push the items in reverse order that we read them so we use
// recursion to invert the order that items are pushed.
static bool log_load_delta_item(struct log *log, struct save *save, world_ts ack)
{
    world_ts time = save_read_type(save, typeof(time));
    if (!time) return true;

    struct coord star = coord_from_u64(save_read_type(save, uint64_t));
    im_id id = save_read_type(save, typeof(id));
    vm_word key = save_read_type(save, typeof(key));
    vm_word value = save_read_type(save, typeof(value));

    if (!log_load_delta_item(log, save, ack)) return false;

    if (time > ack) {
        log_push(log, (struct log_line) {
                .star = star,
                .time = time,
                .id = id,
                .key = key,
                .value = value
            });
    }

    return true;
}

bool log_load_delta(struct log *log, struct save *save, world_ts ack)
{
    if (!save_read_magic(save, save_magic_log)) return false;
    if (!ack) log_reset(log);

    log_load_delta_item(log, save, ack);

    return save_read_magic(save, save_magic_log);
}
