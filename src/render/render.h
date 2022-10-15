/* render.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "game/protocol.h"

#include "SDL.h"
#include <stdatomic.h>


struct proxy;
struct map;
struct factory;
struct ui_topbar;
struct ui_status;
struct ui_tapes;
struct ui_mods;
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
    EV_MOD_BREAKPOINT,

    EV_STARS_TOGGLE,
    EV_STAR_SELECT,
    EV_STAR_CLEAR,

    EV_LOG_TOGGLE,
    EV_LOG_SELECT,

    EV_ITEM_SELECT,
    EV_ITEM_CLEAR,

    EV_IO_TOGGLE,
    EV_ENERGY_TOGGLE,

    EV_MAN_TOGGLE,
    EV_MAN_GOTO,

    EV_IO,

    EV_MAX,
};


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct render
{
    bool init;

    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    pthread_t thread;
    atomic_bool join;

    uint32_t event;
    uint64_t ticks;
    bool focus;

    struct proxy *proxy;
    struct proxy_pipe *pipe;

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
        struct ui_log *log;
        struct ui_stars *stars;
        struct ui_star *star;
        struct ui_item *item;
        struct ui_io *io;
        struct ui_energy *energy;
        struct ui_man *man;
    } ui;
};

extern struct render render;

void render_init(struct proxy *);
void render_close(void);

void render_loop(void);
bool render_done(void);
void render_fork(void);
void render_join(void);

// Unlike EV_UPDATE_STATE which is called once per frame, this will be called
// for every single state update received by the proxy which can be anywhere
// from 1 to 100+ times per frame. As such, it should be limited to things that
// need to see all the intermediary steps and can't just live with whatever is
// current state.
void render_update_state(void);

void render_push_event(enum event, uint64_t d0, uint64_t d1);
void render_push_quit(void);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void render_log_msg(enum status_type, const char *msg, size_t len);
void render_logv(enum status_type, const char *fmt, va_list);
void render_log(enum status_type, const char *fmt, ...) legion_printf(2, 3);
