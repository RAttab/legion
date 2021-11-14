/* state.h
   Rémi Attab (remi.attab@gmail.com), 30 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/world.h"
#include "game/sim.h"
#include "game/tech.h"
#include "utils/htable.h"

struct vec64;
struct log;
struct chunk;
struct atoms;
struct mods_list;
struct mod;
struct save;


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct state
{
    seed_t seed;
    world_ts_t time;
    enum sim_speed speed;
    struct coord home;

    struct atoms *atoms;
    struct mods_list *mods;
    struct vec64 *chunks;
    struct htable lanes;
    struct htable names;
    struct tech tech;
    struct log *log;
    const struct mod *compile;

    struct
    {
        mod_t id;
        const struct mod *mod;
    } mod;

    struct
    {
        struct coord coord;
        struct chunk *chunk;
    } chunk;

};

struct state *state_alloc(void);
void state_free(struct state *);

struct state_ctx
{
    struct world *world;
    enum sim_speed speed;
    struct coord home;

    mod_t mod;
    struct coord chunk;
    const struct mod *compile;
};

void state_save(struct save *, const struct state_ctx *);
bool state_load(struct state *, struct save *);
