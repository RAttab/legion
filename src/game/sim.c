/* sim.c
   Rémi Attab (remi.attab@gmail.com), 28 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sim.h"
#include "game/save.h"
#include "game/state.h"
#include "game/chunk.h"
#include "utils/time.h"

#include <stdatomic.h>
#include <pthread.h>

static void *sim_run(void *ctx);
static void sim_publish(struct sim *);

// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

enum
{
    sim_cmd_len = 8,
    sim_log_len = 16,
    sim_save_version = 1,

    sim_prof_enabled = 1,
    sim_prof_freq = 100,
};

struct sim
{
    pthread_t thread;
    ts_t next;

    struct ack ack;

    struct world *world;
    struct coord home;
    enum speed speed;

    struct coord chunk;

    struct { world_ts_t ts; mod_t id; } mod;
    struct { world_ts_t ts; const struct mod *mod; } compile;

    atomic_bool quit;

    struct
    {
        atomic_size_t read, write;
        struct sim_log queue[sim_log_len];
    } legion_aligned(8) log;

    struct
    {
        atomic_size_t read, write;
        struct sim_cmd queue[sim_cmd_len];
    } legion_aligned(8) cmd;

    struct
    {
        atomic_uintptr_t read, write;
    } legion_aligned(8) state;
};


struct sim *sim_new(seed_t seed)
{
    struct sim *sim = calloc(1, sizeof(*sim));

    sim->world = world_new(seed);
    sim->home = world_populate(sim->world);
    sim->chunk = sim->home;
    sim->speed = speed_normal;

    atomic_init(&sim->quit, false);

    atomic_init(&sim->cmd.read, 0);
    atomic_init(&sim->cmd.write, 0);

    atomic_init(&sim->state.write, (uintptr_t) save_mem_new());
    sim_publish(sim);

    int err = pthread_create(&sim->thread, NULL, sim_run, sim);
    if (err) {
        err_posix(err, "unable to create sim thread: fn=%p ctx=%p", sim_run, sim);
        goto fail_pthread;
    }

    return sim;

  fail_pthread:
    free(sim);
    return NULL;
}

void sim_close(struct sim *sim)
{
    atomic_store_explicit(&sim->quit, true, memory_order_relaxed);

    int err = pthread_join(sim->thread, NULL);
    if (err) err_posix0(err, "unable to join sim thread");

    world_free(sim->world);
    save_mem_free((struct save *) atomic_load(&sim->state.read));
    save_mem_free((struct save *) atomic_load(&sim->state.write));
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------
// Single producer & Single Consumer lockfree queue.


static struct sim_log * sim_log_write(struct sim *sim)
{
    size_t read = atomic_load_explicit(&sim->log.read, memory_order_acquire);
    size_t write = atomic_load_explicit(&sim->log.write, memory_order_relaxed);

    assert(write >= read);
    if (write - read >= sim_log_len) return NULL;

    return sim->log.queue + (write % sim_log_len);
}

static void sim_log_push(struct sim *sim)
{
    size_t write = atomic_fetch_add_explicit(&sim->log.write, 1, memory_order_release);

    size_t read = atomic_load_explicit(&sim->log.read, memory_order_relaxed);
    assert(write >= read);
    assert(write - read <= sim_log_len);
}

const struct sim_log * sim_log_read(struct sim *sim)
{
    size_t write = atomic_load_explicit(&sim->log.write, memory_order_acquire);
    size_t read = atomic_load_explicit(&sim->log.read, memory_order_relaxed);

    assert(write >= read);
    if (write == read) return NULL;

    return sim->log.queue + (read % sim_log_len);
}

void sim_log_pop(struct sim *sim)
{
    size_t read = atomic_fetch_add_explicit(&sim->log.read, 1, memory_order_release);

    size_t write = atomic_load_explicit(&sim->log.write, memory_order_relaxed);
    assert(write >= read);
    assert(write - read <= sim_log_len);
}

static void sim_logv_overflow(enum status type, const char *fmt, va_list args)
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

    fprintf(stderr, "OVERFLOW: <%s> %s\n", prefix, msg);
}

void sim_logv(struct sim *sim, enum status type, const char *fmt, va_list args)
{
    struct sim_log *log = sim_log_write(sim);
    if (!log) return sim_logv_overflow(type, fmt, args);

    ssize_t len = vsnprintf(log->msg, sizeof(log->msg), fmt, args);
    assert(len >= 0);
    log->len = len;

    sim_log_push(sim);
}

void sim_log(struct sim *sim, enum status type, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    sim_logv(sim, type, fmt, args);
    va_end(args);
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
// Single producer & Single Consumer lockfree queue.

struct sim_cmd * sim_cmd_write(struct sim *sim)
{
    size_t read = atomic_load_explicit(&sim->cmd.read, memory_order_acquire);
    size_t write = atomic_load_explicit(&sim->cmd.write, memory_order_relaxed);

    assert(write >= read);
    if (write - read >= sim_cmd_len) return NULL;

    return sim->cmd.queue + (write % sim_cmd_len);
}

void sim_cmd_push(struct sim *sim)
{
    size_t write = atomic_fetch_add_explicit(&sim->cmd.write, 1, memory_order_release);

    size_t read = atomic_load_explicit(&sim->cmd.read, memory_order_relaxed);
    assert(write >= read);
    assert(write - read <= sim_cmd_len);
}

static const struct sim_cmd * sim_cmd_read(struct sim *sim)
{
    size_t write = atomic_load_explicit(&sim->cmd.write, memory_order_acquire);
    size_t read = atomic_load_explicit(&sim->cmd.read, memory_order_relaxed);

    assert(write >= read);
    if (write == read) return NULL;

    return sim->cmd.queue + (read % sim_cmd_len);
}

static void sim_cmd_pop(struct sim *sim)
{
    size_t read = atomic_fetch_add_explicit(&sim->cmd.read, 1, memory_order_release);

    size_t write = atomic_load_explicit(&sim->cmd.write, memory_order_relaxed);
    assert(write >= read);
    assert(write - read <= sim_cmd_len);
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------
// Weird custom handoff mechanism that's mostly lockfree and should work fairly
// well while avoiding having to reallocate the persistence objects.

struct save *sim_state_read(struct sim *sim)
{
    uintptr_t ptr = atomic_exchange_explicit(&sim->state.read, 0, memory_order_acquire);
    assert(!ptr || ptr > 0x100);

    struct save *save = (struct save *) ptr;
    if (save) save_mem_reset(save);
    return save;
}

void sim_state_release(struct sim *sim, struct save *save)
{
    if (!save) return;

    uintptr_t ptr = (uintptr_t) save;
    assert(!ptr || ptr > 0x100);
    ptr = atomic_exchange_explicit(&sim->state.write, ptr, memory_order_release);
    assert(!ptr);
    assert(!ptr || ptr > 0x100);
}

static struct save *sim_state_write(struct sim *sim)
{
    uintptr_t ptr = 0;
    do {
        ptr = atomic_exchange_explicit(&sim->state.write, 0, memory_order_acquire);
        assert(!ptr || ptr > 0x100);
    } while (!ptr && !atomic_load_explicit(&sim->quit, memory_order_relaxed));
    assert(!ptr || ptr > 0x100);

    struct save *save = (struct save *) ptr;
    if (save) save_mem_reset(save);
    return save;
}

static void sim_state_publish(struct sim *sim, struct save *save)
{
    assert(save);

    uintptr_t ptr = (uintptr_t) save;
    assert(!ptr || ptr > 0x100);
    ptr = atomic_exchange_explicit(&sim->state.read, ptr, memory_order_acquire);
    assert(!ptr || ptr > 0x100);
    if (ptr) atomic_store_explicit(&sim->state.write, ptr, memory_order_relaxed);
}


// -----------------------------------------------------------------------------
// thread
// -----------------------------------------------------------------------------

static void sim_cmd_save(struct sim *sim, const struct sim_cmd *cmd)
{
    (void) cmd;

    struct save *save = save_file_new("./legion.save", sim_save_version);

    save_write_magic(save, save_magic_sim);
    save_write_value(save, sim->speed);
    save_write_magic(save, save_magic_sim);

    world_save(sim->world, save);

    size_t bytes = save_len(save);
    save_file_close(save);

    sim_log(sim, st_info, "saved %zu bytes", bytes);
}

static void sim_cmd_load(struct sim *sim, const struct sim_cmd *cmd)
{
    (void) cmd;

    bool fail = false;
    struct save *save = save_file_load("./legion.save");
    assert(save_version(save) == sim_save_version);

    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }
    save_read_into(save, &sim->speed);
    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }

    struct world *world = world_load(save);
    size_t bytes = save_len(save);
    if (!world) { fail = true; goto fail; }

    world = legion_xchg(&sim->world, world);
    world_free(world);

  fail:
    if (fail)
        sim_log(sim, st_error, "save file is corrupted");
    else sim_log(sim, st_info, "loaded %zu bytes", bytes);

    save_file_close(save);
}

static void sim_cmd_io(struct sim *sim, const struct sim_cmd *cmd)
{
    if (cmd->data.io.len > 4) {
        return sim_log(sim, st_error, "invalid length '%u' for IO command '%x:%s'",
                cmd->data.io.len, cmd->data.io.io,
                sim_log_atom(sim, cmd->data.io.io).c);
    }

    struct chunk *chunk = world_chunk(sim->world, sim->chunk);
    if (!chunk) {
        return sim_log(sim, st_error, "invalid star '%s' for IO command '%x:%s'",
                sim_log_coord(sim->chunk).c,
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
        return sim_log(sim, st_error, "invalid dst '%s' for IO command '%x:%s'",
                sim_log_id(cmd->data.io.dst).c,
                cmd->data.io.io,
                sim_log_atom(sim, cmd->data.io.io).c);
    }

    sim_log(sim, st_info, "IO command '%x:%s' sent to '%s'",
            cmd->data.io.io,
            sim_log_atom(sim, cmd->data.io.io).c,
            sim_log_id(cmd->data.io.dst).c);
}

static void sim_cmd_mod(struct sim *sim, const struct sim_cmd *cmd)
{
    if (!mods_get(world_mods(sim->world), cmd->data.mod)) {
        return sim_log(sim, st_error, "unknown mod id '%u.%u'",
                mod_maj(cmd->data.mod),
                mod_ver(cmd->data.mod));
    }

    sim->mod.id = cmd->data.mod;
    sim->mod.ts = world_time(sim->world);
}

static void sim_cmd_mod_register(struct sim *sim, const struct sim_cmd *cmd)
{
    mod_t id = mods_register(world_mods(sim->world), &cmd->data.mod_register);

    if (!id) {
        return sim_log(sim, st_error, "unable to register mod '%s'",
                cmd->data.mod_register.c);
    }

    sim_log(sim, st_error, "mod '%s' registered with id '%u.%u'",
            cmd->data.mod_register.c,
            mod_maj(id), mod_ver(id));
}

static void sim_cmd_mod_compile(struct sim *sim, const struct sim_cmd *cmd)
{
    if (sim->compile.mod) mod_free(sim->compile.mod);

    struct mod *compile = mod_compile(
            cmd->data.mod_compile.maj,
            cmd->data.mod_compile.code,
            cmd->data.mod_compile.len,
            world_mods(sim->world),
            world_atoms(sim->world));
    compile->id = make_mod(cmd->data.mod_compile.maj, 0);
    free((char *) cmd->data.mod_compile.code);

    sim->compile.mod = compile;
    sim->compile.ts = world_time(sim->world);

    if (compile->errs_len)
        return sim_log(sim, st_warn, "%u compilation errors", compile->errs_len);
    sim_log(sim, st_info, "Compilation finished");

}

static void sim_cmd_publish(struct sim *sim, const struct sim_cmd *cmd)
{
    struct mods *mods = world_mods(sim->world);
    const struct mod *mod = sim->compile.mod;

    if (!mod || mod_maj(mod->id) != cmd->data.mod_publish.maj) {
        return sim_log(sim, st_error, "unable to publish compiled mod: %x != %x",
                mod ? mod_maj(mod->id) : 0, cmd->data.mod_publish.maj);
    }

    if (mod->errs_len) {
        return sim_log(sim, st_error, "unable to publish mod with errors: %u",
                mod->errs_len);
    }

    mod_t mod_id = mods_set(mods, mod_maj(mod->id), mod);
    if (!mod_id) {
        return sim_log(sim, st_error, "failed to publish mod '%s'",
                sim_log_mod(sim, mod_id).c);
    }

    sim->compile.mod = NULL;
    sim->mod.id = mod_id;
    sim->mod.ts = world_time(sim->world);
    sim_log(sim, st_error, "mod '%s' published with id '%u.%u'",
            sim_log_mod(sim, mod_id).c, mod_maj(mod_id), mod_ver(mod_id));
}

static void sim_publish(struct sim *sim)
{
    struct save *save = sim_state_write(sim);
    if (!save) return;

    if (unlikely(sim_prof_enabled && core.init)) {
        if (!(world_time(sim->world) % sim_prof_freq))
            save_prof(save);
    }

    state_save(save, &(struct state_ctx) {
                .world = sim->world,
                .speed = sim->speed,
                .home = sim->home,
                .chunk = sim->chunk,
                .mod = sim->mod.ts >= sim->ack.time ? sim->mod.id : 0,
                .compile = sim->compile.ts >= sim->ack.time ? sim->compile.mod : NULL,
                .ack = &sim->ack,
            });

    save_prof_dump(save);
    sim_state_publish(sim, save);
}

static bool sim_cmd(struct sim *sim)
{
    const struct sim_cmd *cmd = NULL;
    while ((cmd = sim_cmd_read(sim)))
    {
        switch (cmd->type)
        {
        case CMD_QUIT: { return false; }
        case CMD_ACK: { sim->ack = cmd->data.ack; break; }

        case CMD_SAVE: { sim_cmd_save(sim, cmd); break; }
        case CMD_LOAD: { sim_cmd_load(sim, cmd); break; }

        case CMD_SPEED: {
            sim->speed = cmd->data.speed;
            if (sim->speed == speed_normal) sim->next = ts_now();
            break;
        }

        case CMD_CHUNK: { sim->chunk = cmd->data.chunk; break; }

        case CMD_MOD: { sim_cmd_mod(sim, cmd); break; }
        case CMD_MOD_REGISTER: { sim_cmd_mod_register(sim, cmd); break; }
        case CMD_MOD_COMPILE: { sim_cmd_mod_compile(sim, cmd); break; }
        case CMD_MOD_PUBLISH: { sim_cmd_publish(sim, cmd); break; }

        case CMD_IO: { sim_cmd_io(sim, cmd); break; }

        default: { assert(false); }
        }

        sim_cmd_pop(sim);
    }

    return true;
}

static void *sim_run(void *ctx)
{
    struct sim *sim = ctx;

    enum { state_freq = 10 };
    ts_t sleep = ts_sec / state_freq;
    ts_t now = sim->next = ts_now();

    while (true) {
        if (atomic_load_explicit(&sim->quit, memory_order_relaxed)) break;

        switch (sim->speed) {
        case speed_fast: { break; }
        case speed_pause: { if (!sim_cmd(sim)) break; continue; }
        case speed_normal: {
            if (now < sim->next)
                now = ts_sleep_until(sim->next);
            break;
        }
        default: { assert(false); }
        }

        // Makes commands mildly more responsive if we execute the step first.
        world_step(sim->world);
        if (!sim_cmd(sim)) break;
        sim_publish(sim);

        sim->next += sleep;
        if (sim->next <= now) {
            dbg("sim.late> now:%lu, next:%lu, sleep:%lu, ticks:%u",
                    now, sim->next, sleep, world_time(sim->world));
            while (sim->next <= now) sim->next += sleep;
        }
    }

    return NULL;
}
