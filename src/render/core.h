/* core.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "utils/time.h"
#include "ui/ui.h"
#include "SDL.h"

struct map;
struct sector;
struct atoms;
struct ui_topbar;


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
    EV_IO_EXEC,

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
    bool focus;

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

    struct
    {
        bool loading;
        ts_t next, sleep;
        struct world *world;
        struct coord home;
    } state;
};

extern struct core core;

void core_init(void);
void core_close(void);

void core_path_res(const char *name, char *dst, size_t len);

void core_run(void);
void core_quit(void);

void core_push_event(enum event, uint64_t d0, uint64_t d1);

void core_save(void);
void core_load(void);
void core_populate(void);

enum status
{
    st_info = 0,
    st_warn = 1,
    st_error = 2,
};

void core_logv(enum status, const char *fmt, va_list);
void core_log(enum status, const char *fmt, ...) legion_printf(2, 3);
