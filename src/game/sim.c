/* sim.c
   Rémi Attab (remi.attab@gmail.com), 28 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "game/save.h"
#include "game/protocol.h"
#include "game/chunk.h"
#include "utils/time.h"

#include <stdatomic.h>
#include <pthread.h>

static void sim_cmd_load(struct sim *);
static struct sim_pipe *sim_pipe_next(struct sim *sim, struct sim_pipe *start);


// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

enum
{
    sim_freq_slow = 10,
    sim_freq_fast = 100,

    sim_log_len = 8,

    sim_save_version = 1,

    sim_prof_enabled = 0,
    sim_prof_freq = 100,
};

struct sim
{
    pthread_t thread;
    atomic_bool join;
    ts_t next;

    struct world *world;
    struct coord home;
    enum speed speed;

    uint64_t stream;

    atomic_uintptr_t pipes;
};


struct sim *sim_new(seed_t seed)
{
    struct sim *sim = calloc(1, sizeof(*sim));

    sim->world = world_new(seed);
    sim->home = world_populate(sim->world);
    sim->speed = speed_slow;
    sim->stream = ts_now();
    atomic_init(&sim->join, false);
    return sim;
}

struct sim *sim_load(void)
{
    struct sim *sim = sim_new(0);
    sim_cmd_load(sim);
    return sim;
}


void sim_free(struct sim *sim)
{
    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
        sim_pipe_close(pipe);

    // We free as we iterate so doing an iteration should free everything. A bit
    // weird but simpler then writing it out
    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
    {}

    world_free(sim->world);
    free(sim);
}


// -----------------------------------------------------------------------------
// pipes
// -----------------------------------------------------------------------------

struct sim_pipe
{
    atomic_uintptr_t next;
    atomic_bool closed;
    struct save_ring *in, *out;

    struct ack *ack;
    struct coord chunk;
    struct { world_ts_t ts; mod_t id; } mod;
    struct { world_ts_t ts; const struct mod *mod; } compile;

    struct
    {
        uint64_t read, write;
        struct status queue[sim_log_len];
    } log;
};

struct sim_pipe *sim_pipe_new(struct sim *sim)
{
    struct sim_pipe *pipe = calloc(1, sizeof(*pipe));
    *pipe = (struct sim_pipe) {
        .in = save_ring_new(sim_in_len),
        .out = save_ring_new(sim_out_len),
        .ack = ack_new(),
        .chunk = sim->home,
    };

    uintptr_t next = atomic_load_explicit(&sim->pipes, memory_order_acquire);
    do {
        atomic_store_explicit(&pipe->next, next, memory_order_relaxed);
    } while (!atomic_compare_exchange_weak_explicit(
                    &sim->pipes, &next, (uintptr_t) pipe,
                    memory_order_acq_rel,
                    memory_order_relaxed));
    return pipe;
}

static void sim_pipe_free(struct sim_pipe *pipe)
{
    if (!pipe) return;

    if (pipe->ack) ack_free(pipe->ack);
    if (pipe->compile.mod) mod_free(pipe->compile.mod);

    save_ring_free(pipe->in);
    save_ring_free(pipe->out);

    free(pipe);
}

// To avoid use-after-free problems we defer the close operation to the
// sim_pipe_next function which is only used in the sim thread. After the call
// the poll thread should never refer to the object again.
void sim_pipe_close(struct sim_pipe *pipe)
{
    atomic_store_explicit(&pipe->closed, true, memory_order_release);
}

struct save_ring *sim_pipe_in(struct sim_pipe *pipe)
{
    return pipe->in;
}

struct save_ring *sim_pipe_out(struct sim_pipe *pipe)
{
    return pipe->out;
}

// The close mechanism relies on the following assumptions:
//
// - After calling sim_pipe_close, the poll thread never touches the object
// - sim_pipe_next is only called from a single thread; the sim thread
// - There are no nested calls to sim_pipe_next
//
// If these assumptions are met then we can be sure that while traversing the
// pipe list, sim_pipe_next holds the only reference to the pipe and it's
// therefore safe to close.
static struct sim_pipe *sim_pipe_next(struct sim *sim, struct sim_pipe *start)
{
    atomic_uintptr_t *it = start ? &start->next : &sim->pipes;
    uintptr_t next = atomic_load_explicit(it, memory_order_acquire);

    while (true) {
        struct sim_pipe *pipe = (void *) next;
        if (!pipe) return NULL;
        if (!atomic_load_explicit(&pipe->closed, memory_order_acquire))
            return pipe;

        uintptr_t new = atomic_load_explicit(&pipe->next, memory_order_acquire);
        bool ok = atomic_compare_exchange_weak_explicit(
                        it, &next, new,
                        memory_order_acq_rel,
                        memory_order_relaxed);
        if (ok) {
            sim_pipe_free(pipe);
            next = new;
        }
    }
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------
// Need to maintain a seperate log queue so that we can accumulate status
// messages and transmit them after we've queued the state.

static struct status *sim_log_write(struct sim_pipe *pipe)
{
    assert(pipe->log.write >= pipe->log.read);
    if (pipe->log.write - pipe->log.read >= sim_log_len) return NULL;
    return pipe->log.queue + (pipe->log.write % sim_log_len);
}

static void sim_log_push(struct sim_pipe *pipe)
{
    pipe->log.write++;
    assert(pipe->log.write >= pipe->log.read);
    assert(pipe->log.write - pipe->log.read <= sim_log_len);
}

static const struct status *sim_log_read(struct sim_pipe *pipe)
{
    assert(pipe->log.write >= pipe->log.read);
    if (pipe->log.write == pipe->log.read) return NULL;

    return pipe->log.queue + (pipe->log.read % sim_log_len);
}

static void sim_log_pop(struct sim_pipe *pipe)
{
    pipe->log.read++;
    assert(pipe->log.write >= pipe->log.read);
    assert(pipe->log.write - pipe->log.read <= sim_log_len);
}

static void sim_logv_overflow(enum status_type type, const char *fmt, va_list args)
{
    static char msg[256] = {0};
    ssize_t len = vsnprintf(msg, sizeof(msg), fmt, args);
    assert(len >= 0);

    const char *prefix = NULL;
    switch (type) {
    case st_info: { prefix = "inf"; break; }
    case st_warn: { prefix = "wrn"; break; }
    case st_error: { prefix = "err"; break; }
    default: { assert(false); }
    }

    errf("log overflow: <%s> %s\n", prefix, msg);
}

static void sim_logv(
        struct sim_pipe *pipe,
        enum status_type type,
        const char *fmt,
        va_list args)
{
    struct status *status = sim_log_write(pipe);
    if (!status) return sim_logv_overflow(type, fmt, args);

    ssize_t len = vsnprintf(status->msg, sizeof(status->msg), fmt, args);
    assert(len >= 0);
    status->len = len;

    sim_log_push(pipe);
}

static legion_printf(3, 4)
void sim_log(struct sim_pipe *pipe, enum status_type type, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    sim_logv(pipe, type, fmt, args);
    va_end(args);
}

static legion_printf(3, 4)
void sim_log_all(struct sim *sim, enum status_type type, const char *fmt, ...)
{
    va_list args = {0};

    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
    {
        va_start(args, fmt);
        sim_logv(pipe, type, fmt, args);
        va_end(args);
    }
}


static struct symbol sim_log_id(id_t id)
{
    struct symbol str = {0};
    id_str(id, str.c, sizeof(str.c));
    return str;
}

static struct symbol sim_log_coord(struct coord coord)
{
    struct symbol str = {0};
    coord_str(coord, str.c, sizeof(str.c));
    return str;
}

static struct symbol sim_log_mod(struct sim *sim, mod_t mod)
{
    struct symbol str = {0};
    mods_name(world_mods(sim->world), mod, &str);
    return str;
}

static struct symbol sim_log_atom(struct sim *sim, word_t atom)
{
    struct symbol str = {0};
    atoms_str(world_atoms(sim->world), atom, &str);
    return str;
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void sim_cmd_save(struct sim *sim)
{
    struct save *save = save_file_create("./legion.save", sim_save_version);

    save_write_magic(save, save_magic_sim);
    save_write_value(save, sim->speed);
    save_write_magic(save, save_magic_sim);

    world_save(sim->world, save);

    size_t bytes = save_len(save);
    save_file_close(save);

    sim_log_all(sim, st_info, "saved %zu bytes", bytes);
}

static void sim_cmd_load(struct sim *sim)
{
    bool fail = false;
    struct save *save = save_file_load("./legion.save");
    assert(save_file_version(save) == sim_save_version);

    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }
    save_read_into(save, &sim->speed);
    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }

    struct world *world = world_load(save);
    size_t bytes = save_len(save);
    if (!world) { fail = true; goto fail; }

    world = legion_xchg(&sim->world, world);
    world_free(world);

    sim->stream++;

  fail:
    if (fail)
        sim_log_all(sim, st_error, "save file is corrupted");
    else sim_log_all(sim, st_info, "loaded %zu bytes", bytes);

    save_file_close(save);
}

static void sim_cmd_io(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (cmd->data.io.len > 4) {
        return sim_log(pipe, st_error, "invalid length '%u' for IO command '%x:%s'",
                cmd->data.io.len, cmd->data.io.io,
                sim_log_atom(sim, cmd->data.io.io).c);
    }

    struct chunk *chunk = world_chunk(sim->world, pipe->chunk);
    if (!chunk) {
        return sim_log(pipe, st_error, "invalid star '%s' for IO command '%x:%s'",
                sim_log_coord(pipe->chunk).c,
                cmd->data.io.io,
                sim_log_atom(sim, cmd->data.io.io).c);
    }

    bool ok = chunk_io(
            chunk,
            cmd->data.io.io,
            0,
            cmd->data.io.dst,
            cmd->data.io.args,
            cmd->data.io.len);

    if (!ok) {
        return sim_log(pipe, st_error, "invalid dst '%s' for IO command '%x:%s'",
                sim_log_id(cmd->data.io.dst).c,
                cmd->data.io.io,
                sim_log_atom(sim, cmd->data.io.io).c);
    }

    sim_log(pipe, st_info, "IO command '%x:%s' sent to '%s'",
            cmd->data.io.io,
            sim_log_atom(sim, cmd->data.io.io).c,
            sim_log_id(cmd->data.io.dst).c);
}

static void sim_cmd_mod(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (!mods_get(world_mods(sim->world), cmd->data.mod)) {
        return sim_log(pipe, st_error, "unknown mod id '%u.%u'",
                mod_maj(cmd->data.mod),
                mod_ver(cmd->data.mod));
    }

    pipe->mod.id = cmd->data.mod;
    pipe->mod.ts = world_time(sim->world);
}

static void sim_cmd_mod_register(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    mod_t id = mods_register(world_mods(sim->world), &cmd->data.mod_register);

    if (!id) {
        return sim_log(pipe, st_error, "unable to register mod '%s'",
                cmd->data.mod_register.c);
    }

    sim_log(pipe, st_error, "mod '%s' registered with id '%u.%u'",
            cmd->data.mod_register.c,
            mod_maj(id), mod_ver(id));
}

static void sim_cmd_mod_compile(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (pipe->compile.mod) mod_free(pipe->compile.mod);

    struct mod *compile = mod_compile(
            cmd->data.mod_compile.maj,
            cmd->data.mod_compile.code,
            cmd->data.mod_compile.len,
            world_mods(sim->world),
            world_atoms(sim->world));
    compile->id = make_mod(cmd->data.mod_compile.maj, 0);
    free((char *) cmd->data.mod_compile.code);

    pipe->compile.mod = compile;
    pipe->compile.ts = world_time(sim->world);

    if (compile->errs_len)
        return sim_log(pipe, st_warn, "%u compilation errors", compile->errs_len);
    sim_log(pipe, st_info, "Compilation finished");
}

static void sim_cmd_publish(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    struct mods *mods = world_mods(sim->world);
    const struct mod *mod = pipe->compile.mod;

    if (!mod || mod_maj(mod->id) != cmd->data.mod_publish.maj) {
        return sim_log(pipe, st_error, "unable to publish compiled mod: %x != %x",
                mod ? mod_maj(mod->id) : 0, cmd->data.mod_publish.maj);
    }

    if (mod->errs_len) {
        return sim_log(pipe, st_error, "unable to publish mod with errors: %u",
                mod->errs_len);
    }

    mod_t mod_id = mods_set(mods, mod_maj(mod->id), mod);
    if (!mod_id) {
        return sim_log(pipe, st_error, "failed to publish mod '%s'",
                sim_log_mod(sim, mod_id).c);
    }

    pipe->compile.mod = NULL;
    pipe->mod.id = mod_id;
    pipe->mod.ts = world_time(sim->world);
    sim_log(pipe, st_error, "mod '%s' published with id '%u.%u'",
            sim_log_mod(sim, mod_id).c, mod_maj(mod_id), mod_ver(mod_id));
}

static void sim_cmd(struct sim *sim, struct sim_pipe *pipe)
{
    while (true) {
        struct save *save = save_ring_read(pipe->in);

        struct header head = {0};
        if (save_read(save, &head, sizeof(head)) != sizeof(head)) break;

        if (head.magic != header_magic) {
            sim_log(pipe, st_error, "invalid head magic: %x != %x",
                    head.magic, header_magic);
            save_ring_close(pipe->in);
            return;
        }

        if (head.type != header_cmd) {
            sim_log(pipe, st_error, "unexpected input header type: %u != %d",
                    head.type, header_cmd);
            save_ring_consume(save, head.len);
            goto commit;
        }

        if (save_cap(save) < head.len) break;

        struct cmd cmd = {0};
        if (!cmd_load(&cmd, save)) {
            sim_log(pipe, st_error, "unable to load cmd '%x'", cmd.type);
            goto commit;
        }

        switch (cmd.type)
        {
        case CMD_QUIT: {
            atomic_store_explicit(&sim->join, true, memory_order_relaxed);
            break;
        }

        case CMD_SAVE: { sim_cmd_save(sim); break; }
        case CMD_LOAD: { sim_cmd_load(sim); break; }

        case CMD_ACK: {
            if (pipe->ack) ack_free(pipe->ack);
            pipe->ack = legion_xchg(&cmd.data.ack, NULL);
            break;
        }

        case CMD_SPEED: {
            sim->speed = cmd.data.speed;
            if (sim->speed != speed_pause) sim->next = ts_now();
            break;
        }

        case CMD_CHUNK: { pipe->chunk = cmd.data.chunk; break; }

        case CMD_MOD: { sim_cmd_mod(sim, pipe, &cmd); break; }
        case CMD_MOD_REGISTER: { sim_cmd_mod_register(sim, pipe, &cmd); break; }
        case CMD_MOD_COMPILE: { sim_cmd_mod_compile(sim, pipe, &cmd); break; }
        case CMD_MOD_PUBLISH: { sim_cmd_publish(sim, pipe, &cmd); break; }

        case CMD_IO: { sim_cmd_io(sim, pipe, &cmd); break; }

        default: { assert(false); }
        }

      commit:
        save_ring_commit(pipe->in, save);
    }
}

static void sim_cmd_all(struct sim *sim)
{
    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
        sim_cmd(sim, pipe);
}


// -----------------------------------------------------------------------------
// publish
// -----------------------------------------------------------------------------

static void sim_publish_state(struct sim *sim, struct sim_pipe *pipe)
{
    struct save *save = save_ring_write(pipe->out);

    struct header *head = save_bytes(save);
    if (save_ring_consume(save, sizeof(*head)) != sizeof(*head)) {
        sim_log(pipe, st_warn, "skip state publish: %zu", save_cap(save));
        return;
    }

    if (unlikely(sim_prof_enabled && render.init)) {
        if (!(world_time(sim->world) % sim_prof_freq))
            save_prof(save);
    }

    bool ack_mod = pipe->ack->time > pipe->mod.ts;
    bool ack_compile = pipe->ack->time > pipe->compile.ts;
    if (sim->stream != pipe->ack->stream) ack_reset(pipe->ack);

    state_save(save, &(struct state_ctx) {
                .stream = sim->stream,
                .world = sim->world,
                .speed = sim->speed,
                .home = sim->home,
                .chunk = pipe->chunk,
                .mod = !ack_mod ? pipe->mod.id : 0,
                .compile = !ack_compile ? pipe->compile.mod : NULL,
                .ack = pipe->ack,
            });

    // If this triggers, increase the ring size or detect eof in chunk and
    // do some magical form of gradual state transmit. Short version, life
    // not bueno.
    assert(save_len(save) < sim_out_len);

    if (save_eof(save)) {
        sim_log(pipe, st_warn, "skip state publish: %zu", save_cap(save));
        return;
    }

    save_prof_dump(save);

    *head = make_header(header_state, save_len(save));
    save_ring_commit(pipe->out, save);
    save_ring_wake_signal(pipe->out);
}

static void sim_publish_log(struct sim_pipe *pipe)
{
    const struct status *status = NULL;
    while ((status = sim_log_read(pipe))) {
        struct save *save = save_ring_write(pipe->out);

        struct header *head = save_bytes(save);
        if (save_ring_consume(save, sizeof(*head)) != sizeof(*head)) {
            sim_log(pipe, st_warn, "skip log publish: %zu", save_cap(save));
            return;
        }

        status_save(status, save);

        if (save_len(save) == save_cap(save)) {
            sim_log(pipe, st_warn, "skip log publish: %zu", save_cap(save));
            return;
        }

        *head = make_header(header_status, save_len(save));
        save_ring_commit(pipe->out, save);
        sim_log_pop(pipe);
    }
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

void sim_step(struct sim *sim)
{
    world_step(sim->world);

    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
    {
        sim_cmd(sim, pipe);

        sim_publish_state(sim, pipe);
        sim_publish_log(pipe);

        save_ring_wake_signal(pipe->out);
    }
}

void sim_loop(struct sim *sim)
{
    const ts_t sleep_slow = ts_sec / sim_freq_slow;
    const ts_t sleep_fast = ts_sec / sim_freq_fast;
    ts_t now = sim->next = ts_now();

    while (true) {
        if (atomic_load_explicit(&sim->join, memory_order_relaxed)) break;

        ts_t sleep = 0;
        switch (sim->speed) {
        case speed_pause: { sim_cmd_all(sim); continue; }
        case speed_slow: { sleep = sleep_slow; break; }
        case speed_fast: { sleep = sleep_fast; break; }
        default: { assert(false); }
        }

        if (now < sim->next)
            now = ts_sleep_until(sim->next);

        sim_step(sim);

        sim->next += sleep;
        if (sim->next <= now) {
            if (sim->speed == speed_slow) {
                dbgf("sim.late: now=%lu, next=%lu, sleep=%lu, ticks=%u",
                        now, sim->next, sleep, world_time(sim->world));
            }
            sim->next = now;
        }
    }
}

void sim_join(struct sim *sim)
{
    atomic_store_explicit(&sim->join, true, memory_order_relaxed);

    int err = pthread_join(sim->thread, NULL);
    if (err) err_posix(err, "unable to join sim thread");
}

void sim_thread(struct sim *sim)
{
    void *sim_run(void *ctx)  { sim_loop(ctx); return NULL; }

    int err = pthread_create(&sim->thread, NULL, sim_run, sim);
    if (!err) return;

    failf_posix(err, "unable to create sim thread: fn=%p ctx=%p",
            sim_run, sim);
}
