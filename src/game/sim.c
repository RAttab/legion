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
#include "utils/config.h"

#include <stdatomic.h>
#include <pthread.h>

static struct sim_pipe *sim_pipe_next(struct sim *sim, struct sim_pipe *start);
static void sim_publish_mod(struct sim_pipe *pipe, const struct mod *mod);


// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

enum
{
    sim_freq_slow = 10,
    sim_freq_fast = 100,
    sim_freq_faster = 1000,

    sim_log_len = 8,

    sim_save_version = 1,

    sim_prof_enabled = 0,
    sim_prof_freq = 100,
};

struct sim
{
    pthread_t thread;
    atomic_bool join, reload;
    time_sys next;

    struct world *world;
    struct users users;
    enum speed speed;

    world_ts autosave;

    bool server;
    uint64_t stream;
    atomic_uintptr_t pipes;

    char save[PATH_MAX + 1];
    char config[PATH_MAX + 1];
};

struct sim *sim_new(world_seed seed, const char *save)
{
    struct sim *sim = calloc(1, sizeof(*sim));
    sim->world = world_new(seed);
    world_populate(sim->world);
    users_init(&sim->users);
    sim->speed = speed_slow;
    sim->stream = ts_now();
    atomic_init(&sim->join, false);
    atomic_init(&sim->reload, false);
    strncpy(sim->save, save, sizeof(sim->save) - 1);
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
    users_free(&sim->users);
    free(sim);
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

static void sim_config_write(struct sim *sim)
{
    struct config config = {0};
    struct writer *out = config_write(&config, sim->config);

    writer_open_nl(out);
    writer_symbol_str(out, "server");
    writer_field(out, "autosave", word, sim->autosave);
    writer_close(out);

    users_write(&sim->users, out);
    config_close(&config);
}

static void sim_config_read(struct sim *sim)
{
    struct config config = {0};
    struct reader *in = config_read(&config, sim->config);

    reader_open(in);
    reader_symbol_str(in, "server");
    sim->autosave = reader_field(in, "autosave", word);
    reader_close(in);

    users_read(&sim->users, in);
    config_close(&config);
}

void sim_server(struct sim *sim, const char *config)
{
    sim->server = true;
    strncpy(sim->config, config, sizeof(sim->config) - 1);
    sim_config_read(sim);
}

void sim_server_reload(struct sim *sim)
{
    atomic_store_explicit(&sim->reload, true, memory_order_relaxed);
}


// -----------------------------------------------------------------------------
// pipes
// -----------------------------------------------------------------------------

struct sim_pipe
{
    atomic_uintptr_t next;
    atomic_bool closed;
    struct save_ring *in, *out;

    struct { bool ok; struct user user; } auth;

    struct ack *ack;
    struct coord chunk;
    const struct mod *compile;

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
    };

    if (!sim->server) {
        pipe->auth.ok = true;
        pipe->auth.user = *users_id(&sim->users, user_admin);
        pipe->chunk = world_home(sim->world, user_admin);
    }

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
    mod_free(pipe->compile);

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

    errf("log overflow: <%s> %s", prefix, msg);
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
    status->type = type;

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


static struct symbol sim_log_id(im_id id)
{
    struct symbol str = {0};
    im_id_str(id, str.c, sizeof(str.c));
    return str;
}

static struct symbol sim_log_coord(struct coord coord)
{
    struct symbol str = {0};
    coord_str(coord, str.c, sizeof(str.c));
    return str;
}

static struct symbol sim_log_mod(struct sim *sim, mod_id mod)
{
    struct symbol str = {0};
    mods_name(world_mods(sim->world), mod_major(mod), &str);
    return str;
}

static struct symbol sim_log_atom(struct sim *sim, vm_word atom)
{
    struct symbol str = {0};
    atoms_str(world_atoms(sim->world), atom, &str);
    return str;
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

void sim_save(struct sim *sim)
{
    struct save *save = save_file_create(sim->save, sim_save_version);

    save_write_magic(save, save_magic_sim);
    save_write_value(save, sim->speed);
    save_write_magic(save, save_magic_sim);

    world_save(sim->world, save);

    size_t bytes = save_len(save);
    save_file_close(save);

    sim_log_all(sim, st_info, "saved %zu bytes", bytes);
    infof("saved %zu bytes", bytes);
}

void sim_load(struct sim *sim)
{
    struct save *save = save_file_load(sim->save);
    if (!save) {
        sim_log_all(sim, st_error, "unable to open '%s'", sim->save);
        return;
    }

    bool fail = false;
    if (save_file_version(save) != sim_save_version) { fail = true; goto fail; }

    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }
    save_read_into(save, &sim->speed);
    if (!save_read_magic(save, save_magic_sim)) { fail = true; goto fail; }

    struct world *world = world_load(save);
    size_t bytes = save_len(save);
    if (!world) { fail = true; goto fail; }

    world = legion_xchg(&sim->world, world);
    world_free(world);

    sim->stream = ts_now();

  fail:
    if (fail)
        sim_log_all(sim, st_error, "save file is corrupted");
    else sim_log_all(sim, st_info, "loaded %zu bytes", bytes);

    save_file_close(save);
}

static void sim_cmd_user_response(struct sim_pipe *pipe)
{
    struct save *save = save_ring_write(pipe->out);

    struct header *head = save_bytes(save);
    if (save_ring_consume(save, sizeof(*head)) != sizeof(*head)) {
        sim_log(pipe, st_warn, "unable to return auth info: %zu", save_cap(save));
        return;
    }

    user_save(&pipe->auth.user, save);

    *head = make_header(header_user, save_len(save));
    save_ring_commit(pipe->out, save);
    save_ring_wake_signal(pipe->out);
}

static void sim_cmd_user(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (!users_auth_server(&sim->users, cmd->data.user.server)) {
        infof("failed server auth from '%s' with '%lx'",
                cmd->data.user.name.c, cmd->data.user.server);
        sim_log(pipe, st_error, "invalid server token");
        save_ring_close(pipe->out);
        return;
    }

    const struct user *user = users_create(&sim->users, &cmd->data.user.name);
    if (!user) {
        infof("failed to create user '%s'", cmd->data.user.name.c);
        sim_log(pipe, st_error, "unable to create user '%s'",
                cmd->data.user.name.c);
        save_ring_close(pipe->out);
        return;
    }

    world_populate_user(sim->world, user->id);
    pipe->chunk = world_home(sim->world, user->id);
    pipe->auth.user = *user;
    pipe->auth.ok = true;

    sim_cmd_user_response(pipe);
    sim_config_write(sim);

    infof("user '%u:%s' created", user->id, cmd->data.user.name.c);
}

static void sim_cmd_auth(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (!users_auth_server(&sim->users, cmd->data.auth.server)) {
        infof("failed server auth from '%u' with '%lx'",
                cmd->data.auth.id, cmd->data.auth.server);
        sim_log(pipe, st_error, "invalid server token");
        save_ring_close(pipe->out);
        return;
    }

    const struct user *user = users_auth_user(
            &sim->users, cmd->data.auth.id, cmd->data.auth.private);

    if (!user) {
        infof("failed to auth user '%u'", cmd->data.auth.id);
        sim_log(pipe, st_error, "unable to login as user '%u'",
                cmd->data.auth.id);
        save_ring_close(pipe->out);
        return;
    }

    // There's a chance that the user state wasn't saved in which case creating
    // a new state is our best option. Function is a noop if the user is already
    // populated.
    world_populate_user(sim->world, user->id);

    pipe->chunk = world_home(sim->world, user->id);
    pipe->auth.user = *user;
    pipe->auth.ok = true;

    sim_cmd_user_response(pipe);
    infof("user '%u:%s' authed", user->id, user->name.c);
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
    const struct mod *mod = mods_get(world_mods(sim->world), cmd->data.mod);
    if (!mod) {
        return sim_log(pipe, st_error, "unknown mod id '%u.%u'",
                mod_major(cmd->data.mod),
                mod_version(cmd->data.mod));
    }

    sim_publish_mod(pipe, mod);
}

static void sim_cmd_mod_register(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    mod_id id = mods_register(
            world_mods(sim->world),
            pipe->auth.user.id,
            &cmd->data.mod_register);

    if (!id) {
        return sim_log(pipe, st_error, "unable to register mod '%s'",
                cmd->data.mod_register.c);
    }

    sim_log(pipe, st_info, "mod '%s' registered with id '%u.%u'",
            cmd->data.mod_register.c,
            mod_major(id),
            mod_version(id));
}

static void sim_cmd_mod_compile(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    if (pipe->compile) mod_free(pipe->compile);

    struct mod *compile = mod_compile(
            cmd->data.mod_compile.maj,
            cmd->data.mod_compile.code,
            cmd->data.mod_compile.len,
            world_mods(sim->world),
            world_atoms(sim->world));
    compile->id = make_mod(cmd->data.mod_compile.maj, 0);
    free((char *) cmd->data.mod_compile.code);

    pipe->compile = compile;
    sim_publish_mod(pipe, pipe->compile);

    if (pipe->compile->errs_len)
        return sim_log(pipe, st_warn, "%u compilation errors", compile->errs_len);
    sim_log(pipe, st_info, "Compilation finished");
}

static void sim_cmd_publish(
        struct sim *sim, struct sim_pipe *pipe, const struct cmd *cmd)
{
    struct mods *mods = world_mods(sim->world);
    const struct mod *mod = pipe->compile;

    if (!mod || mod_major(mod->id) != cmd->data.mod_publish.maj) {
        return sim_log(pipe, st_error, "unable to publish compiled mod: %x != %x",
                mod ? mod_major(mod->id) : 0, cmd->data.mod_publish.maj);
    }

    if (mod->errs_len) {
        return sim_log(pipe, st_error, "unable to publish mod with errors: %u",
                mod->errs_len);
    }

    mod_id mod_id = mods_set(mods, mod_major(mod->id), mod);
    if (!mod_id) {
        return sim_log(pipe, st_error, "failed to publish mod '%s'",
                sim_log_mod(sim, mod_id).c);
    }

    sim_publish_mod(pipe, mod);

    pipe->compile = NULL;
    sim_log(pipe, st_info, "mod '%s' published with id '%u.%u'",
            sim_log_mod(sim, mod_id).c,
            mod_major(mod_id),
            mod_version(mod_id));
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

        if (!pipe->auth.ok && !(cmd.type == CMD_USER || cmd.type == CMD_AUTH)) {
            sim_log(pipe, st_error, "user not authenticated");
            save_ring_close(pipe->out);
            goto commit;
        }

        switch (cmd.type)
        {
        case CMD_QUIT: {
            atomic_store_explicit(&sim->join, true, memory_order_relaxed);
            break;
        }

        // \todo should probably restrict to admin only.
        case CMD_SAVE: { sim_save(sim); break; }
        case CMD_LOAD: { sim_load(sim); break; }

        case CMD_USER: { sim_cmd_user(sim, pipe, &cmd); break; }
        case CMD_AUTH: { sim_cmd_auth(sim, pipe, &cmd); break; }

        case CMD_ACK: {
            if (pipe->ack) ack_free(pipe->ack);
            pipe->ack = legion_xchg(&cmd.data.ack, NULL);
            break;
        }

        // \todo should probably restrict to admin only.
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

    if (sim->stream != pipe->ack->stream) ack_reset(pipe->ack);

    struct user *user = &pipe->auth.user;
    state_save(save, &(struct state_ctx) {
                .stream = sim->stream,
                .access = user->access,
                .user = user->id,
                .world = sim->world,
                .speed = sim->speed,
                .chunk = pipe->chunk,
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

static void sim_publish_mod(struct sim_pipe *pipe, const struct mod *mod)
{
    struct save *save = save_ring_write(pipe->out);
    struct header *head = save_bytes(save);

    if (save_ring_consume(save, sizeof(*head)) != sizeof(*head)) {
        sim_log(pipe, st_warn, "skip log publish: %zu", save_cap(save));
        return;
    }

    mod_save(mod, save);
    if (save_len(save) == save_cap(save)) {
        sim_log(pipe, st_warn, "skip mod publish: %zu", save_cap(save));
        return;
    }

    *head = make_header(header_mod, save_len(save));
    save_ring_commit(pipe->out, save);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

void sim_step(struct sim *sim)
{
    if (sim->speed != speed_pause)
        world_step(sim->world);

    for (struct sim_pipe *pipe = sim_pipe_next(sim, NULL);
         pipe; pipe = sim_pipe_next(sim, pipe))
    {
        sim_cmd(sim, pipe);
        if (pipe->auth.ok) sim_publish_state(sim, pipe);
        sim_publish_log(pipe);

        save_ring_wake_signal(pipe->out);
    }
}

void sim_loop(struct sim *sim)
{
    time_sys now = sim->next = ts_now();
    while (!atomic_load_explicit(&sim->join, memory_order_relaxed)) {

        // Should eventually get more complicated as we might need to close some
        // pipes if we've invalidated active users but keep it simple for now.
        if (atomic_exchange_explicit(&sim->reload, false, memory_order_relaxed))
            sim_config_read(sim);

        time_sys sleep = 0;
        switch (sim->speed) {
        case speed_pause:
        case speed_slow:    { sleep = ts_sec / sim_freq_slow; break; }
        case speed_fast:    { sleep = ts_sec / sim_freq_fast; break; }
        case speed_faster:  { sleep = ts_sec / sim_freq_faster; break; }
        case speed_fastest: { sleep = 0; break; }
        default: { assert(false); }
        }

        if (now < sim->next)
            now = ts_sleep_until(sim->next);

        sim_step(sim);

        if (sim->autosave && world_time(sim->world) % sim->autosave == 0)
            sim_save(sim);

        sim->next += sleep;
        if (sim->next <= now) {
            if (sim->speed == speed_slow) {
                infof("sim.late: now=%lu, next=%lu, sleep=%lu, ticks=%u",
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

void sim_fork(struct sim *sim)
{
    void *sim_run(void *ctx)  { sim_loop(ctx); return NULL; }

    int err = pthread_create(&sim->thread, NULL, sim_run, sim);
    if (!err) return;

    failf_posix(err, "unable to create sim thread: fn=%p ctx=%p",
            sim_run, sim);
}
