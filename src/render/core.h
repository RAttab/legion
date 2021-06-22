/* core.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/time.h"
#include "SDL.h"

struct map;
struct ui_topbar;
struct sector;


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum event
{
    EV_NIL = 0,

    EV_STATE_UPDATE,

    EV_MODS_TOGGLE,

    EV_MOD_SELECT,
    EV_MOD_CLEAR,

    EV_STAR_SELECT,
    EV_STAR_CLEAR,

    EV_ITEM_SELECT,
    EV_ITEM_CLEAR,

    EV_FOCUS,

    EV_MAX,
};


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct core
{
    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    uint32_t event;
    uint64_t ticks;

    struct {
        SDL_Point point;
        SDL_Texture *tex;
        size_t size;
    } cursor;

    struct {
        struct map *map;
        struct ui_topbar *topbar;
        struct ui_mods *mods;
        struct ui_mod *mod;
        struct ui_star *star;
    } ui;

    struct {
        ts_t next, sleep;
        uint64_t time;
        struct sector *sector;
    } state;
};

extern struct core core;

void core_init();
void core_close();

void core_path_res(const char *name, char *dst, size_t len);

void core_run();
void core_quit();

void core_push_event(enum event, uint64_t d0, uint64_t d1);
