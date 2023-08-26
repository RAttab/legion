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


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

// int is required to cleanly compare against the signed value in the SDL_Event
// object.
enum event : int
{
    ev_nil = 0,

    ev_state_load,

    ev_frame,
    ev_focus_panel,
    ev_focus_input,

    ev_star_select,
    ev_item_select,
    ev_tape_select,

    ev_max,
};


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void render_init(void);
void render_close(void);

bool render_initialized(void);

SDL_Rect render_rect(void);
struct dim render_dim(void);

enum event render_user_event(const SDL_Event *);
bool render_user_event_is(const SDL_Event *, enum event);
SDL_Renderer *render_renderer(void);

void render_loop(void);
bool render_done(void);
void render_fork(void);
void render_join(void);

void render_push_event(enum event, uint64_t d0, uint64_t d1);
void render_quit(void);
