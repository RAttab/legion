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
static void proxy_pipe_free(struct proxy *, struct proxy_pipe *);
static void proxy_cmd(struct proxy *, const struct cmd *);


// -----------------------------------------------------------------------------
// proxy
// -----------------------------------------------------------------------------

struct proxy
{
    struct save_ring *in, *out;
    const struct mod *mod;
    struct state *state;
    world_seed seed;

    struct htable sectors;
    struct hset *active_stars;
    struct hset *active_sectors;
    struct lisp *lisp;

    atomic_uintptr_t pipe;

    struct {
        user_token server;
        struct user user;
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
    if (pipe) proxy_pipe_free(proxy, pipe);

    mod_free(proxy->mod);
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

    writer_open_nl(out);
    writer_symbol(out, &proxy->auth.user.name);
    user_write(&proxy->auth.user, out);
    writer_close(out);

    writer_close(out);
    config_close(&config);
}

static void proxy_config_read(struct proxy *proxy)
{
    struct config config = {0};
    struct reader *in = config_read(&config, proxy->config);

    reader_open(in);
    reader_symbol_str(in, "client");
    proxy->auth.server = reader_field(in, "server", u64);

    reader_open(in);
    proxy->auth.user.name = reader_symbol(in);
    if (!reader_peek_close(in)) user_read(&proxy->auth.user, in);
    reader_close(in);

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
    atomic_bool closed;

    struct sim_pipe *sim;
    struct save_ring *in, *out;

    struct ack *ack;
    struct coord chunk;
};

bool proxy_pipe_ready(struct proxy *proxy)
{
    return atomic_load_explicit(&proxy->pipe, memory_order_relaxed) == 0;
}

struct proxy_pipe *proxy_pipe_new(struct proxy *proxy, struct sim_pipe *sim)
{
    struct proxy_pipe *pipe = calloc(1, sizeof(*pipe));
    *pipe = (struct proxy_pipe) {
        .sim = sim,
        .in = sim ? sim_pipe_out(sim) : save_ring_new(sim_out_len),
        .out = sim ? sim_pipe_in(sim) : save_ring_new(sim_in_len),
        .ack = ack_new(),
    };

    assert(proxy_pipe_ready(proxy));
    atomic_store_explicit(&proxy->pipe, (uintptr_t) pipe, memory_order_release);

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
                            .name = proxy->auth.user.name,
                        }}});
    }

    return pipe;
}

static void proxy_pipe_free(struct proxy *proxy, struct proxy_pipe *pipe)
{
    struct proxy_pipe *old = (void *) atomic_exchange_explicit(
            &proxy->pipe, 0, memory_order_relaxed);
    assert(pipe == old);

    if (pipe->ack) ack_free(pipe->ack);
    if (!pipe->sim) { // in local mode the pipe is shared and owned by sim.
        save_ring_free(pipe->in);
        save_ring_free(pipe->out);
    }

    free(pipe);
}

static bool proxy_pipe_closed(struct proxy_pipe *pipe)
{
    return atomic_load_explicit(&pipe->closed, memory_order_acquire);
}

void proxy_pipe_close(struct proxy_pipe *pipe)
{
    atomic_store_explicit(&pipe->closed, true, memory_order_release);
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
    return (void *) atomic_load_explicit(&proxy->pipe, memory_order_acquire);
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

static void proxy_update_state_io(struct proxy *proxy, struct world_io *io)
{
    if (!io->io) return;

    char id_str[im_id_str_len] = {0};
    im_id_str(io->src, id_str, sizeof(id_str));

    struct symbol io_str = {0};
    bool ok = atoms_str(proxy_atoms(proxy), io->io, &io_str);
    assert(ok);

    if (!io->len) {
        render_log(st_info, "received IO command '%s' from '%s'",
                io_str.c, id_str);
        return;
    }

    enum { arg_str_len = 16 };
    char arg_str[(2 + arg_str_len + 1) * 4 + 1] = {0};

    char *it = arg_str;
    for (size_t i = 0; i < io->len; ++i) {
        *it = '0'; it++;
        *it = 'x'; it++;
        it += str_utox(io->args[i], it, arg_str_len);
        *it = ' '; it++;
    }
    *it = '\0';
    assert((uintptr_t)(it - arg_str) <= sizeof(arg_str));

    render_log(st_info, "received IO command '%s' from '%s' with [ %s]",
            io_str.c, id_str, arg_str);
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
    proxy_update_state_io(proxy, &proxy->state->io);

    return stream && proxy->state->stream != stream ?
        proxy_loaded : proxy_updated;
}

static void proxy_update_user(struct proxy *proxy, struct save *save)
{
    if (user_load(&proxy->auth.user, save))
        proxy_config_write(proxy);
    else err("unable to load auth information");
}

static enum proxy_ret proxy_update_mod(struct proxy *proxy, struct save *save)
{
    const struct mod *mod = mod_load(save);
    if (!mod) {
        err("unable to load mod information");
        return proxy_nil;
    }

    mod_free(legion_xchg(&proxy->mod, mod));
    return proxy_updated;
}

enum proxy_ret proxy_update(struct proxy *proxy)
{
    struct proxy_pipe *pipe = proxy_pipe(proxy);
    if (!pipe) return false;

    enum proxy_ret result = proxy_nil;
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
        enum proxy_ret ret = proxy_nil;

        switch (head.type) {
        case header_status: { proxy_update_status(proxy, save); break; }
        case header_state: { ret = proxy_update_state(proxy, pipe, save); break; }
        case header_user: { proxy_update_user(proxy, save); break; }
        case header_mod: { ret = proxy_update_mod(proxy, save); break; }
        default: { assert(false); }
        }

        if (result != proxy_loaded && ret != proxy_nil) result = ret;
        save_ring_commit(pipe->in, save);
    }

    // if the pipe was reset make sure to restore our subscriptions
    if (!coord_eq(pipe->chunk, proxy->state->chunk.coord))
        proxy_chunk(proxy, proxy->state->chunk.coord);

    if (proxy_pipe_closed(pipe))
        proxy_pipe_free(proxy, pipe);

    return result;
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

world_seed proxy_seed(struct proxy *proxy)
{
    return proxy->state->seed;
}

world_ts proxy_time(struct proxy *proxy)
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

struct lisp *proxy_lisp(struct proxy *proxy)
{
    return proxy->lisp;
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

void proxy_io(
        struct proxy *proxy,
        enum io io, im_id dst,
        const vm_word *args, uint8_t len)
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

const struct mod *proxy_mod(struct proxy *proxy)
{
    return legion_xchg(&proxy->mod, (const struct mod *) NULL);
}

void proxy_mod_select(struct proxy *proxy, mod_id id)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD,
                .data = { .mod = id },
            });
}

void proxy_mod_register(struct proxy *proxy, struct symbol name)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_REGISTER,
                .data = { .mod_register = name },
            });
}

void proxy_mod_publish(struct proxy *proxy, mod_maj maj)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_PUBLISH,
                .data = { .mod_publish = { .maj = maj } },
            });
}

mod_id proxy_mod_latest(struct proxy *proxy, mod_maj maj)
{
    struct mods_list *mods = proxy->state->mods;

    for (size_t i = 0; mods->len; ++i) {
        struct mods_item *it = mods->items + i;
        if (it->maj == maj) return make_mod(it->maj, it->ver);
    }

    return 0;
}

bool proxy_mod_name(struct proxy *proxy, mod_maj maj, struct symbol *dst)
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

void proxy_mod_compile(struct proxy *proxy, mod_maj maj, const char *code, size_t len)
{
    proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_MOD_COMPILE,
                .data = { .mod_compile = { .maj = maj, .code = code, .len = len } },
            });
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


vm_word proxy_star_name(struct proxy *proxy, struct coord coord)
{
    struct htable_ret ret = htable_get(&proxy->state->names, coord_to_u64(coord));
    if (ret.ok) return (vm_word) ret.value;

    return gen_name_star(coord, proxy->state->seed, proxy->state->atoms);
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
