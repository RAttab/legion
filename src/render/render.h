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
struct ui;


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

enum event
{
    ev_nil = 0,

    ev_state_load,

    ev_frame,
    ev_focus_panel,
    ev_focus_input,

    ev_star_select,
    ev_item_select,
    ev_tape_select,
    ev_mod_select,

    ev_max,
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
    uint64_t frames;

    struct proxy *proxy;
    struct proxy_pipe *pipe;

    struct ui *ui;
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
void render_quit(void);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void render_log_msg(enum status_type, const char *msg, size_t len);
void render_logv(enum status_type, const char *fmt, va_list);
void render_log(enum status_type, const char *fmt, ...) legion_printf(2, 3);
