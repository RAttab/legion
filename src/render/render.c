/* render.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/render.h"
#include "render/font.h"
#include "render/ui.h"
#include "game/sys.h"
#include "game/tape.h"
#include "game/coord.h"
#include "game/tape.h"
#include "game/proxy.h"
#include "items/config.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "utils/err.h"
#include "utils/time.h"
#include "db/res.h"

#include <pthread.h>


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct
{
    bool init;

    SDL_Rect rect;
    SDL_Window *window;
    SDL_Renderer *renderer;

    pthread_t thread;
    atomic_bool join;

    uint32_t event;
    uint64_t frames;
} render;


void render_init(void)
{
    render.init = true;

    sdl_err(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    SDL_DisplayMode display = {0};
    sdl_err(SDL_GetDesktopDisplayMode(0, &display));
    render.rect = (SDL_Rect) { .x = 0, .y = 0, .w = display.w, .h = display.h };

    sdl_err(SDL_CreateWindowAndRenderer(
                    render.rect.w, render.rect.h,
                    SDL_WINDOW_FULLSCREEN,
                    &render.window, &render.renderer));

    render.event = SDL_RegisterEvents(1);
    if (render.event == (uint32_t) -1) sdl_fail();

    fonts_populate(render.renderer);
    ui_init();
}

void render_close(void)
{
    fonts_close();
    ui_free();

    SDL_DestroyRenderer(render.renderer);
    SDL_DestroyWindow(render.window);
    SDL_Quit();
}

bool render_initialized(void)
{
    return render.init;
}

SDL_Rect render_rect(void)
{
    return render.rect;
}

struct dim render_dim(void)
{
    return make_dim(render.rect.w, render.rect.h);
}

enum event render_user_event(const SDL_Event *ev)
{
    return ev->type == render.event ? ev->user.code : ev_nil;
}

bool render_user_event_is(const SDL_Event *ev, enum event type)
{
    return render_user_event(ev) && ev->user.code == type;
}

SDL_Renderer *render_renderer(void)
{
    return render.renderer;
}


// -----------------------------------------------------------------------------
// thread
// -----------------------------------------------------------------------------

static bool render_step(void)
{
    if (atomic_load_explicit(&render.join, memory_order_relaxed))
        return false;

    // Should not depend on whether proxy has an update or not.
    ui_cursor_update();

    switch (proxy_update())
    {
    case proxy_nil: { break; }
    case proxy_loaded: {
        ui_reset();
        render_push_event(ev_state_load, 0, 0);
    } // fallthrough
    case proxy_updated: { ui_update_frame(); break; }
    default: { assert(false); }
    }

    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        ui_event(&event);
    }

    sdl_err(SDL_RenderClear(render.renderer));
    ui_render(render.renderer);
    SDL_RenderPresent(render.renderer);

    return true;
}

void render_loop(void)
{
    enum { fps_cap = 60 };
    time_sys sleep = ts_sec / fps_cap;

    time_sys ts = ts_now();
    while (render_step()) {
        ts = ts_sleep_until(ts + sleep);

        render.frames++;
        render_push_event(ev_frame, render.frames, 0);
    }

    atomic_store_explicit(&render.join, true, memory_order_relaxed);
}

bool render_done(void)
{
    return atomic_load_explicit(&render.join, memory_order_relaxed);
}

void render_fork(void)
{
    void *render_run(void *) { render_loop(); return NULL; }

    int err = pthread_create(&render.thread, NULL, render_run, NULL);
    if (!err) return;

    failf_posix(err, "unable to create render thread: fn=%p", render_run);
}

void render_join(void)
{
    atomic_store_explicit(&render.join, true, memory_order_relaxed);

    int err = pthread_join(render.thread, NULL);
    if (err) err_posix(err, "unable to join render thread");
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

void render_push_event(enum event code, uint64_t d0, uint64_t d1)
{
    SDL_Event ev = {0};
    ev.type = render.event;
    ev.user.code = code;
    ev.user.data1 = (void *) d0;
    ev.user.data2 = (void *) d1;

    int ret = sdl_err(SDL_PushEvent(&ev));
    assert(ret > 0);
}

void render_quit(void)
{
    // Prevents log spam as we're exiting.
    proxy_set_speed(speed_pause);

    sdl_err(SDL_PushEvent(&(SDL_Event){ .type = SDL_QUIT }));
}
