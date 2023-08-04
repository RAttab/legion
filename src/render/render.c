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

struct render render = {0};


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

static void ui_init(void)
{
    ui_style_default();
    ui_clipboard_init();
    render.ui = ui_alloc();
}

static void ui_close(void)
{
    ui_free(render.ui);
    ui_clipboard_free();
}

void render_update_state(void)
{
    // We don't want to execute this when running in tests.
    if (!render.init) return;

    // worker & energy
    ui_update_state(render.ui, render.proxy);
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void render_init(struct proxy *proxy)
{
    render.init = true;
    render.proxy = proxy;

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
    ui_close();

    SDL_DestroyRenderer(render.renderer);
    SDL_DestroyWindow(render.window);
    SDL_Quit();
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

    switch (proxy_update(render.proxy))
    {
    case proxy_nil: { break; }
    case proxy_loaded: {
        ui_reset(render.ui);
        render_push_event(ev_state_load, 0, 0);
    } // fallthrough
    case proxy_updated: { ui_update_frame(render.ui, render.proxy); break; }
    default: { assert(false); }
    }

    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        ui_event(render.ui, &event);
    }

    sdl_err(SDL_RenderClear(render.renderer));
    ui_render(render.ui, render.renderer);
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
    proxy_set_speed(render.proxy, speed_pause);

    sdl_err(SDL_PushEvent(&(SDL_Event){ .type = SDL_QUIT }));
}


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

void render_log_msg(enum status_type type, const char *msg, size_t len)
{
    const char *prefix = NULL;
    switch (type) {
    case st_info: { prefix = "inf"; break; }
    case st_warn: { prefix = "wrn"; break; }
    case st_error: { prefix = "err"; break; }
    default: { assert(false); }
    }

    if (render.init) {
        ui_status_set(type, msg, len);
        fprintf(stderr, "<%s> %s\n", prefix, msg);
    }
}

void render_logv(enum status_type type, const char *fmt, va_list args)
{
    static char msg[256] = {0};
    ssize_t len = vsnprintf(msg, sizeof(msg), fmt, args);
    assert(len >= 0);
    render_log_msg(type, msg, len);
}

void render_log(enum status_type type, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    render_logv(type, fmt, args);
    va_end(args);
}
