/* proxy.c
   Rémi Attab (remi.attab@gmail.com), 29 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/proxy.h"
#include "game/sector.h"
#include "game/protocol.h"
#include "game/chunk.h"
#include "render/core.h"
#include "utils/htable.h"
#include "utils/hset.h"

static void proxy_cmd(struct proxy *, const struct cmd *);


// -----------------------------------------------------------------------------
// proxy
// -----------------------------------------------------------------------------

struct proxy
{
    struct sim *sim;
    struct state *state;

    struct htable sectors;
    struct hset *active_stars;
    struct hset *active_sectors;
    struct lisp *lisp;

    mod_t mod;
    struct coord chunk;

    struct ack ack;
};

struct proxy *proxy_new(struct sim *sim)
{
    struct proxy *proxy = calloc(1, sizeof(*proxy));
    *proxy = (struct proxy) {
        .sim = sim,
        .state = state_alloc(),
    };

    proxy->lisp = lisp_new(proxy->state->mods, proxy->state->atoms);

    bool updated = proxy_update(proxy);
    assert(updated);

    proxy->mod = proxy->state->mod.id;
    proxy->chunk = proxy->state->chunk.coord;

    return proxy;
}

void proxy_free(struct proxy *proxy)
{
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

bool proxy_update(struct proxy *proxy)
{
    bool ok = false;

    struct save *save = sim_state_read(proxy->sim);
    if (save) {
        if ((ok = state_load(proxy->state, save, &proxy->ack))) {
            proxy_cmd(proxy, &(struct cmd) {
                .type = CMD_ACK,
                .data = { .ack = ack_clone(&proxy->ack) },
            });
        }
        else err("unable to load state in proxy");

        sim_state_release(proxy->sim, save);
    }

    const struct sim_log *log = NULL;
    while ((log = sim_log_read(proxy->sim))) {
        core_log_msg(log->type, log->msg, log->len);
        sim_log_pop(proxy->sim);
    }

    if (!ok) return false;

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

    return true;
}

struct lisp_ret proxy_eval(struct proxy *proxy, const char *src, size_t len)
{
    return lisp_eval_const(proxy->lisp, src, len);
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


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void proxy_cmd(struct proxy *proxy, const struct cmd *src)
{
    struct cmd *dst = sim_cmd_write(proxy->sim);
    if (!dst) return core_log(st_error, "unable to send command '%d'", src->type);

    *sim_cmd_write(proxy->sim) = *src;

    sim_cmd_push(proxy->sim);
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
    if (coord_eq(proxy->state->chunk.coord, coord)) {
        assert(coord_is_nil(coord) == !proxy->state->chunk.chunk);
        return proxy->state->chunk.chunk;
    }

    if (coord_eq(proxy->chunk, coord)) return NULL;
    proxy->chunk = coord;

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
    if (proxy->state->mod.id == id) {
        const struct mod *mod = proxy->state->mod.mod;
        proxy->state->mod.mod = NULL;
        proxy->mod = 0;
        return mod;
    }

    if (proxy->mod == id) return NULL;
    proxy->mod = id;

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
