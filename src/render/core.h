/* core.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "game/sim.h"
#include "SDL.h"

struct proxy;
struct map;
struct factory;
struct ui_topbar;
struct ui_status;
struct ui_tapes;
struct ui_mods;
struct ui_mod;
struct ui_log;
struct ui_stars;
struct ui_star;
struct ui_item;
struct ui_io;


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum event
{
    EV_NIL = 0,

    EV_TICK,
    EV_STATE_UPDATE,
    EV_STATE_LOAD,

    EV_FOCUS_PANEL,
    EV_FOCUS_INPUT,

    EV_MAP_GOTO,

    EV_FACTORY_SELECT,
    EV_FACTORY_CLOSE,

    EV_TAPES_TOGGLE,
    EV_TAPE_SELECT,

    EV_MODS_TOGGLE,
    EV_MOD_SELECT,
    EV_MOD_CLEAR,

    EV_STARS_TOGGLE,
    EV_STAR_SELECT,
    EV_STAR_CLEAR,

    EV_LOG_TOGGLE,
    EV_LOG_SELECT,

    EV_ITEM_SELECT,
    EV_ITEM_CLEAR,

    EV_IO_TOGGLE,

    EV_MAX,
};


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct core
{
    bool init;

    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    uint32_t event;
    uint64_t ticks;
    bool focus;

    struct sim *sim;
    struct proxy *proxy;

    struct
    {
        SDL_Point point;
        SDL_Texture *tex;
        size_t size;
    } cursor;

    struct
    {
        struct ui_clipboard board;

        struct map *map;
        struct factory *factory;
        struct ui_topbar *topbar;
        struct ui_status *status;
        struct ui_tapes *tapes;
        struct ui_mods *mods;
        struct ui_mod *mod;
        struct ui_log *log;
        struct ui_stars *stars;
        struct ui_star *star;
        struct ui_item *item;
        struct ui_io *io;
    } ui;
};

extern struct core core;

void core_init(void);
void core_close(void);
void core_populate(void);

void core_path_res(const char *name, char *dst, size_t len);

void core_run(void);
void core_quit(void);

void core_push_event(enum event, uint64_t d0, uint64_t d1);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void core_log_msg(enum status, const char *msg, size_t len);
void core_logv(enum status, const char *fmt, va_list);
void core_log(enum status, const char *fmt, ...) legion_printf(2, 3);
