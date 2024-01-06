/* metrics.c
   Rémi Attab (remi.attab@gmail.com), 06 Jan 2024
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// metrics
// -----------------------------------------------------------------------------

static struct
{
    bool dump;
    struct mfile_writer out;
    struct strbuf buf;
} metrics = {0};

void metrics_open(const char *path)
{
    mfile_writer_open(&metrics.out, path, page_len * 1000);
    strbuf_alloc(&metrics.buf, page_len * 10);
    metrics.dump = true;
}

void metrics_close(void)
{
    mfile_writer_close(&metrics.out);
    strbuf_free(&metrics.buf);
    metrics.dump = false;
}

inline void metric_add(struct metric *m, struct metric v)
{
    m->n += v.n; m->t += v.t;
}

static const char *metric_rate(double value, uint64_t dt)
{
    double rate =
        ((value * dt) / metrics_config_period) /
        (metrics_config_period / sys_sec);
    return strbuf_scaled_f(&metrics.buf, rate);
}

static const char *metric_percent(double value, uint64_t dt)
{
    return strbuf_fmt(&metrics.buf, "%5.3lf", value / dt);
}

void metrics_dump(struct metrics *m)
{
    if (!metrics.dump) return;

    sys_ts now = sys_now();
    if (!m->t.next)
        m->t.next = (m->t.start = now) + metrics_config_period;
    if (now < m->t.next) return;

    const uint64_t dt = now - m->t.start;
    const uint64_t dts = m->ts.now - m->ts.start;

    size_t shards = 0;
    struct metrics_shard sum = {0};

    for (size_t i = 0; i < shards_cap; ++i) {
        struct metrics_shard *shard = m->shard + i;
        if (!shard->active) continue;
        shards++;

        metric_add(&sum.shard.idle, shard->shard.idle);
        metric_add(&sum.shard.chunks, shard->shard.chunks);
        metric_add(&sum.chunk.workers, shard->chunk.workers);
        for (size_t j = 0; j < items_active_len; ++j)
            metric_add(&sum.chunk.active[j], shard->chunk.active[j]);
    }

    size_t items = sum.chunk.workers.n;
    for (size_t i = 0; i < items_active_len; ++i)
        items += sum.chunk.active[i].n;

    struct mfile_writer *out = &metrics.out;
    struct strbuf *buf = strbuf_reset(&metrics.buf);

    mfile_writef(out, "(m (dt %s) (steps %s %s)\n",
            strbuf_scaled(buf, dt),
            strbuf_scaled(buf, dts),
            metric_rate(dts, dt));

    mfile_writef(out, "  (sim (idle %s) (cmd %s) (pub %s))\n",
            metric_percent(m->sim.idle.t, dt),
            metric_percent(m->sim.cmd.t, dt),
            metric_percent(m->sim.publish.t, dt));

    mfile_writef(out, "  (world (items %s) (lanes %s %s))\n",
            metric_rate(items, dt),
            metric_rate(m->world.lanes.n, dt),
            metric_percent(m->world.lanes.t, dt));

    mfile_writef(out, "  (shards (begin %s) (wait %s) (end %s)\n",
            metric_percent(m->shards.begin.t, dt),
            metric_percent(m->shards.wait.t, dt),
            metric_percent(m->shards.end.t, dt));

    void dump_shard(const char *name, const struct metrics_shard *ms, uint64_t div)
    {
        double idle = ((double) ms->shard.idle.t) / sys_sec;
        mfile_writef(out, "    (%s (idle %s %s) (chunks %s %s)\n",
                name,
                metric_percent(ms->shard.idle.t / div, dt),
                metric_rate(idle / div, dt),
                metric_percent(ms->shard.chunks.t / div, dt),
                metric_rate(ms->shard.chunks.n, dt));

        mfile_writef(out, "      (%10s %s %s)\n",
                item_str_c(item_worker),
                metric_rate(ms->chunk.workers.n, dt),
                metric_percent(ms->chunk.workers.t / div, dt));

        for (size_t i = 0; i < array_len(ms->chunk.active); ++i)
            mfile_writef(out, "      (%10s %s %s)\n",
                    item_str_c(items_active_first + i),
                    metric_rate(ms->chunk.active[i].n, dt),
                    metric_percent(ms->chunk.active[i].t / div, dt));

        out->it--;
        mfile_write(out, ")\n");
    }

    dump_shard("all", &sum, shards);
    for (size_t i = 0; i < array_len(m->shard); ++i) {
        const struct metrics_shard *ms = m->shard + i;
        if (ms->active) dump_shard(strbuf_fmt(buf, "%02zu", i), ms, 1);
    }

    out->it--;
    mfile_write(out, "))\n\n");

    world_ts ts = m->ts.now;
    memset(m, 0, sizeof(*m));
    m->t.next = (m->t.start = now) + metrics_config_period;
    m->ts.start = ts;
}
