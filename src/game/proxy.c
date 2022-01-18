/* proxy.c
   Rémi Attab (remi.attab@gmail.com), 29 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/proxy.h"
#include "game/sector.h"
#include "game/protocol.h"
#include "game/chunk.h"
#include "render/render.h"
#include "utils/htable.h"
#include "utils/hset.h"
#include "utils/config.h"

static struct proxy_pipe *proxy_pipe(struct proxy *);
static void proxy_cmd(struct proxy *, const struct cmd *);


// -----------------------------------------------------------------------------
// proxy
// -----------------------------------------------------------------------------

struct proxy
{
    struct save_ring *in, *out;
    struct state *state;
    seed_t seed;

    struct htable sectors;
    struct hset *active_stars;
    struct hset *active_sectors;
    struct lisp *lisp;

    struct { atomic_uintptr_t active, free; } pipe;

    struct {
        token_t server;
        struct user user;
        struct symbol name;
    } auth;

    char config[PATH_MAX + 1];
};

struct proxy *proxy_new(void)
{
    struct proxy *proxy = calloc(1, sizeof(*proxy));
    proxy->state = state_alloc();

    // This is not great but our UI needs atoms to initialize so it's either
    // this or waiting for the first update from the sim which we can't do when
    // in client mode.
    im_populate_atoms(proxy->state->atoms);

    proxy->lisp = lisp_new(proxy->state->mods, proxy->state->atoms);
    return proxy;
}

void proxy_free(struct proxy *proxy)
{
    struct proxy_pipe *pipe = proxy_pipe(proxy);
    if (pipe) proxy_pipe_close(proxy, pipe);
    (void) proxy_pipe(proxy); // frees any closed pipes.

    state_free(proxy->state);
    hset_free(proxy->active_sectors);
    hset_free(proxy->active_stars);
    lisp_free(proxy->lisp);

    for (const struct htable_bucket *it = htable_next(&proxy->sectors, NULL);
         it; it = htable_next(&proxy->sectors, it))
        sector_free((struct sector *) it->value);
    htable_reset(&proxy->sectors);

    free(proxy);
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

static void proxy_config_write(struct proxy *proxy)
{
    struct config config = {0};
    struct writer *out = config_write(&config, proxy->config);

    writer_open(out);
    writer_symbol_str(out, "client");
    writer_field(out, "server", u64, proxy->auth.server);
    writer_field(out, "name", symbol, &proxy->auth.name);
    user_write(&proxy->auth.user, out);
    writer_close(out);

    config_close(&config);
}

static void proxy_config_read(struct proxy *proxy)
{
    struct config config = {0};
    struct reader *in = config_read(&config, proxy->config);

    reader_open(in);
    reader_key(in, "client");
    proxy->auth.server = reader_field(in, "server", u64);
    proxy->auth.name = reader_field(in, "name", symbol);
    if (!reader_peek_close(in)) user_read(&proxy->auth.user, in);
    reader_close(in);

    config_close(&config);
}

void proxy_auth(struct proxy *proxy, const char *config)
{
    strncpy(proxy->config, config, sizeof(proxy->config) - 1);
    proxy_config_read(proxy);
}


// -----------------------------------------------------------------------------
// pipe
// -----------------------------------------------------------------------------

struct proxy_pipe
{
    struct proxy_pipe *next;

    struct sim_pipe *sim;
    struct save_ring *in, *out;

    struct ack *ack;
    struct coord chunk;
    mod_t mod;
};

struct proxy_pipe *proxy_pipe_new(struct proxy *proxy, struct sim_pipe *sim)
{
    struct proxy_pipe *pipe = calloc(1, sizeof(*pipe));
    *pipe = (struct proxy_pipe) {
        .sim = sim,
        .in = sim ? sim_pipe_out(sim) : save_ring_new(sim_out_len),
        .out = sim ? sim_pipe_in(sim) : save_ring_new(sim_in_len),
        .ack = ack_new(),
    };

    uintptr_t old = atomic_exchange_explicit(
            &proxy->pipe.active, (uintptr_t) pipe, memory_order_release);
    assert(!old);

    if (proxy->auth.user.private) {
        proxy_cmd(proxy, &(struct cmd) {
                    .type = CMD_AUTH,
                    .data = {
                        .auth = {
                            .server = proxy->auth.server,
                            .id = proxy->auth.user.id,
                            .private = proxy->auth.user.private,
                        }}});
    }
    else if (proxy->auth.server) {
        proxy_cmd(proxy, &(struct cmd) {
                    .type = CMD_USER,
                    .data = {
                        .user = {
                            .server = proxy->auth.server,
                            .name = proxy->auth.name,
                        }}});
    }

    return pipe;
}

static void proxy_pipe_free(struct proxy_pipe *pipe)
{
    if (!pipe) return;

    if (pipe->ack) ack_free(pipe->ack);
    if (!pipe->sim) { // in local mode the pipe is shared and owned by sim.
        save_ring_free(pipe->in);
        save_ring_free(pipe->out);
    }

    free(pipe);
}

void proxy_pipe_close(struct proxy *proxy, struct proxy_pipe *pipe)
{
    struct proxy_pipe *old = (void *) atomic_exchange_explicit(
            &proxy->pipe.active, (uintptr_t) NULL, memory_order_acquire);
    assert(old == pipe);

    uintptr_t next = atomic_load_explicit(&proxy->pipe.free, memory_order_acquire);
    do {
        pipe->next = (struct proxy_pipe *) next;
    } while (!atomic_compare_exchange_weak_explicit(
                    &proxy->pipe.free, &next, (uintptr_t) pipe,
                    memory_order_acq_rel,
                    memory_order_relaxed));
}

struct save_ring *proxy_pipe_in(struct proxy_pipe *pipe)
{
    return pipe->in;
}

struct save_ring *proxy_pipe_out(struct proxy_pipe *pipe)
{
    return pipe->out;
}

static struct proxy_pipe *proxy_pipe(struct proxy *proxy)
{
    struct proxy_pipe *pipe = (void *) atomic_exchange_explicit(
            &proxy->pipe.free, (uintptr_t) NULL, memory_order_acquire);
    while (pipe) {
        struct proxy_pipe *next = pipe->next;
        proxy_pipe_free(pipe);
        pipe = next;
    }

    return (void *) atomic_load_explicit(&proxy->pipe.active, memory_order_acquire);
}


// -----------------------------------------------------------------------------
// update
// -----------------------------------------------------------------------------

static void proxy_update_status(struct proxy *, struct save *save)
{
    struct status status = {0};
    if (!status_load(&status, save)) return;

    render_log_msg(status.type, status.msg, status.len);
}

static enum proxy_ret proxy_update_state(
        struct proxy *proxy, struct proxy_pipe *pipe, struct save *save)
{
    uint64_t stream = proxy->state->stream;
    if (!state_load(proxy->state, save, pipe->ack)) {
        err("unable to load state in proxy");
        return proxy_nil;
    }

    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_ACK,
                .data = { .ack = pipe->ack }
            });

    if (proxy->seed != proxy->state->seed) {
        proxy->seed = proxy->state->seed;

        for (const struct htable_bucket *it = htable_next(&proxy->sectors, NULL);
             it; it = htable_next(&proxy->sectors, it))
            sector_free((void *) it->value);
        htable_clear(&proxy->sectors);
    }

    hset_clear(proxy->active_stars);
    hset_clear(proxy->active_sectors);

    struct vec64 *chunks = proxy->state->chunks;
    for (size_t i = 0; i < chunks->len; ++i) {
        struct coord star = coord_from_u64(chunks->vals[i]);
        proxy->active_stars = hset_put(proxy->active_stars, coord_to_u64(star));

        struct coord sector = coord_sector(star);
        proxy->active_sectors = hset_put(proxy->active_sectors, coord_to_u64(sector));
    }

    lisp_context(proxy->lisp, proxy->state->mods, proxy->state->atoms);

    return stream == proxy->state->stream ? proxy_updated : proxy_loaded;
}

static void proxy_update_user(struct proxy *proxy, struct save *save)
{
    if (user_load(&proxy->auth.user, save))
        proxy_config_write(proxy);
    else err("unable to load auth information");
}

enum proxy_ret proxy_update(struct proxy *proxy)
{
    struct proxy_pipe *pipe = proxy_pipe(proxy);
    if (!pipe) return false;

    enum proxy_ret ret = proxy_nil;
    while (true) {
        struct save *save = save_ring_read(pipe->in);

        struct header head = {0};
        if (save_read(save, &head, sizeof(head)) != sizeof(head)) break;

        if (head.magic != header_magic) {
            render_log(st_warn, "invalid head magic: %x != %x",
                    head.magic, header_magic);
            break;
        }

        if (save_cap(save) < head.len) break;

        switch (head.type) {
        case header_status: { proxy_update_status(proxy, save); break; }
        case header_state: { ret = proxy_update_state(proxy, pipe, save); break; }
        case header_user: { proxy_update_user(proxy, save); break; }
        default: { assert(false); }
        }

        save_ring_commit(pipe->in, save);
    }

    // if the pipe was reset make sure to restore our subscriptions
    if (!coord_eq(pipe->chunk, proxy->state->chunk.coord))
        proxy_chunk(proxy, proxy->state->chunk.coord);

    return ret;
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

seed_t proxy_seed(struct proxy *proxy)
{
    return proxy->state->seed;
}

world_ts_t proxy_time(struct proxy *proxy)
{
    return proxy->state->time;
}

enum speed proxy_speed(struct proxy *proxy)
{
    return proxy->state->speed;
}

struct coord proxy_home(struct proxy *proxy)
{
    return proxy->state->home;
}

const struct tech *proxy_tech(struct proxy *proxy)
{
    return &proxy->state->tech;
}

struct atoms *proxy_atoms(struct proxy *proxy)
{
    return proxy->state->atoms;
}

const struct mods_list *proxy_mods(struct proxy *proxy)
{
    return proxy->state->mods;
}

const struct vec64 *proxy_chunks(struct proxy *proxy)
{
    return proxy->state->chunks;
}

const struct htable *proxy_lanes(struct proxy *proxy)
{
    return &proxy->state->lanes;
}

const struct hset *proxy_lanes_for(struct proxy *proxy, struct coord star)
{
    struct htable_ret ret = htable_get(&proxy->state->lanes, coord_to_u64(star));
    return ret.ok ? (void *) ret.value : NULL;
}

const struct log *proxy_logs(struct proxy *proxy)
{
    return proxy->state->log;
}

struct lisp_ret proxy_eval(struct proxy *proxy, const char *src, size_t len)
{
    return lisp_eval_const(proxy->lisp, src, len);
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void proxy_cmd(struct proxy *proxy, const struct cmd *cmd)
{
    struct proxy_pipe *pipe = proxy_pipe(proxy);
    if (!pipe) {
        render_log(st_error,
                "unable to send command '%x' while not connected to a server",
                cmd->type);
        return;
    }

    struct save *save = save_ring_write(pipe->out);

    struct header *head = save_bytes(save);
    if (save_ring_consume(save, sizeof(*head)) != sizeof(*head)) {
        render_log(st_error, "unable to send command '%x' due to overflow",
                cmd->type);
        return;
    }

    cmd_save(cmd, save);

    if (save_eof(save)) {
        render_log(st_error, "unable to send command '%x' due to overflow",
                cmd->type);
        return;
    }

    *head = make_header(header_cmd, save_len(save));
    save_ring_commit(pipe->out, save);
    save_ring_wake_signal(pipe->out);
}

void proxy_quit(struct proxy *proxy)
{
    proxy_cmd(proxy, &(struct cmd) { .type = CMD_QUIT });
}

void proxy_save(struct proxy *proxy)
{
    proxy_cmd(proxy, &(struct cmd) { .type = CMD_SAVE });
}

void proxy_load(struct proxy *proxy)
{
    proxy_cmd(proxy, &(struct cmd) { .type = CMD_LOAD });
}

void proxy_set_speed(struct proxy *proxy, enum speed speed)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_SPEED,
                .data = { .speed = speed },
            });
}

struct chunk *proxy_chunk(struct proxy *proxy, struct coord coord)
{
    if (coord_eq(proxy->state->chunk.coord, coord))
        return proxy->state->chunk.chunk;

    struct proxy_pipe *pipe = proxy_pipe(proxy);
    if (!pipe || coord_eq(pipe->chunk, coord)) return NULL;
    pipe->chunk = coord;

    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_CHUNK,
                .data = { .chunk = coord },
            });
    return NULL;
}

void proxy_io(struct proxy *proxy, enum io io, id_t dst, const word_t *args, uint8_t len)
{
    struct cmd cmd = {
        .type = CMD_IO,
        .data = { .io = { .io = io, .dst = dst, .len = len } },
    };
    memcpy(cmd.data.io.args, args, len * sizeof(*args));
    proxy_cmd(proxy, &cmd);
}


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

mod_t proxy_mod_id(struct proxy *proxy)
{
    return proxy->state->mod.id;
}

const struct mod *proxy_mod(struct proxy *proxy, mod_t id)
{
    struct proxy_pipe *pipe = proxy_pipe(proxy);

    if (proxy->state->mod.id == id) {
        const struct mod *mod = proxy->state->mod.mod;
        proxy->state->mod.mod = NULL;
        proxy->state->mod.id = 0;
        if (pipe) pipe->mod = 0;
        return mod;
    }

    if (!pipe || pipe->mod == id) return NULL;
    pipe->mod = id;

    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD,
                .data = { .mod = id },
            });
    return NULL;
}

void proxy_mod_register(struct proxy *proxy, struct symbol name)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_REGISTER,
                .data = { .mod_register = name },
            });
}

void proxy_mod_publish(struct proxy *proxy, mod_maj_t maj)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_PUBLISH,
                .data = { .mod_publish = { .maj = maj } },
            });
}

mod_t proxy_mod_latest(struct proxy *proxy, mod_maj_t maj)
{
    struct mods_list *mods = proxy->state->mods;

    for (size_t i = 0; mods->len; ++i) {
        struct mods_item *it = mods->items + i;
        if (it->maj == maj) return make_mod(it->maj, it->ver);
    }

    return 0;
}

bool proxy_mod_name(struct proxy *proxy, mod_maj_t maj, struct symbol *dst)
{
    struct mods_list *mods = proxy->state->mods;

    for (size_t i = 0; i < mods->len; ++i) {
        struct mods_item *it = mods->items + i;
        if (it->maj != maj) continue;

        *dst = it->str;
        return true;
    }

    return false;
}

void proxy_mod_compile(struct proxy *proxy, mod_maj_t maj, const char *code, size_t len)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_COMPILE,
                .data = { .mod_compile = { .maj = maj, .code = code, .len = len } },
            });
}

const struct mod *proxy_mod_compile_result(struct proxy *proxy)
{
    const struct mod *mod = proxy->state->compile;
    proxy->state->compile = NULL;
    return mod;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct proxy_render_it proxy_render_it(struct proxy *proxy, struct rect viewport)
{
    return (struct proxy_render_it) {
        .rect = viewport,
        .sector = proxy_sector(proxy, viewport.top),
        .index = 0,
    };
}

const struct star *proxy_render_next(struct proxy *proxy, struct proxy_render_it *it)
{
    while (true) {
        if (it->index < it->sector->stars_len) {
            struct star *star = &it->sector->stars[it->index++];
            if (rect_contains(&it->rect, star->coord)) return star;
            continue;
        }

        struct coord coord = rect_next_sector(it->rect, it->sector->coord);
        if (coord_is_nil(coord)) return NULL;
        it->sector = proxy_sector(proxy, coord);
        it->index = 0;
    }
}

struct sector *proxy_sector(struct proxy *proxy, struct coord sector)
{
    struct coord coord = coord_sector(sector);
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&proxy->sectors, id);
    if (ret.ok) return (struct sector *) ret.value;

    struct sector *value = gen_sector(coord, proxy->state->seed);
    ret = htable_put(&proxy->sectors, id, (uintptr_t) value);
    assert(ret.ok);

    return value;
}

bool proxy_active_star(struct proxy *proxy, struct coord coord)
{
    return hset_test(proxy->active_stars, coord_to_u64(coord));
}

bool proxy_active_sector(struct proxy *proxy, struct coord coord)
{
    return hset_test(proxy->active_sectors, coord_to_u64(coord));
}


word_t proxy_star_name(struct proxy *proxy, struct coord coord)
{
    struct htable_ret ret = htable_get(&proxy->state->names, coord_to_u64(coord));
    return ret.ok ? (word_t) ret.value : gen_name(coord, proxy->state->seed, proxy->state->atoms);
}

const struct star *proxy_star_in(struct proxy *proxy, struct rect rect)
{
    // Very likely that the center of the rectangle is the right place to look
    // so make a first initial guess.
    struct sector *sector = proxy_sector(proxy, rect_center(&rect));
    const struct star *star = sector_star_in(sector, rect);
    if (star) return star;

    // Alright so our guess didn't work out so time for an exhaustive search.
    struct coord it = rect_next_sector(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        sector = proxy_sector(proxy, rect_center(&rect));
        star = sector_star_in(sector, rect);
        if (star) return star;
    }

    return NULL;
}

const struct star *proxy_star_at(struct proxy *proxy, struct coord coord)
{
    return sector_star_at(proxy_sector(proxy, coord), coord);
}
