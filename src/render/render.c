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
#include "game/gen.h"
#include "game/tape.h"
#include "game/proxy.h"
#include "items/config.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "utils/err.h"
#include "utils/time.h"

#include <pthread.h>


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct render render = {0};


// -----------------------------------------------------------------------------
// cursor
// -----------------------------------------------------------------------------

static bool cursor_focus(void)
{
    uint32_t flags = SDL_GetWindowFlags(render.window);
    return flags & SDL_WINDOW_INPUT_FOCUS;
}

static void cursor_init(void)
{
    render.cursor.size = 20;
    render.cursor.point = (SDL_Point){
        .x = render.rect.w / 2,
        .y = render.rect.h / 2
    };

    char path[PATH_MAX];
    sys_path_res("cursor.bmp", path, sizeof(path));

    SDL_Surface *surface = sdl_ptr(SDL_LoadBMP(path));
    render.cursor.tex = sdl_ptr(SDL_CreateTextureFromSurface(render.renderer, surface));
    SDL_FreeSurface(surface);

    sdl_err(SDL_SetTextureBlendMode(render.cursor.tex, SDL_BLENDMODE_ADD));
    sdl_err(SDL_SetTextureColorMod(render.cursor.tex, 0xFF, 0xFF, 0xFF));

    render.focus = cursor_focus();
    sdl_err(SDL_SetRelativeMouseMode(render.focus));
}

static void cursor_close(void)
{
    SDL_DestroyTexture(render.cursor.tex);
}

static void cursor_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_MOUSEMOTION: {
        render.cursor.point.x += event->motion.xrel;
        render.cursor.point.x = legion_max(0, legion_min(render.rect.w, render.cursor.point.x));

        render.cursor.point.y += event->motion.yrel;
        render.cursor.point.y = legion_max(0, legion_min(render.rect.h, render.cursor.point.y));
        break;
    }
    }
}

static void cursor_render(SDL_Renderer *renderer)
{
    if (!render.focus) return;
    sdl_err(SDL_RenderCopy(renderer, render.cursor.tex,
                    &(SDL_Rect){ .x = 0, .y = 0, .w = 50, .h = 50 },
                    &(SDL_Rect){
                        .x = render.cursor.point.x,
                        .y = render.cursor.point.y,
                        .w = render.cursor.size,
                        .h = render.cursor.size }));
}

static void cursor_update(void)
{
    bool focus = cursor_focus();
    if (focus == render.focus) return;

    sdl_err(SDL_SetRelativeMouseMode(focus));
    render.focus = focus;
}


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

static void ui_init(void)
{
    ui_clipboard_init(&render.ui.board);

    render.ui.map = map_new();
    render.ui.factory = factory_new();
    render.ui.topbar = ui_topbar_new();
    render.ui.status = ui_status_new();
    render.ui.tapes = ui_tapes_new();
    render.ui.mods = ui_mods_new();
    render.ui.mod = ui_mod_new();
    render.ui.log = ui_log_new();
    render.ui.stars = ui_stars_new();
    render.ui.star = ui_star_new();
    render.ui.item = ui_item_new();
    render.ui.io = ui_io_new();
}

static void ui_close(void)
{
    ui_topbar_free(render.ui.topbar);
    ui_status_free(render.ui.status);
    ui_tapes_free(render.ui.tapes);
    ui_mods_free(render.ui.mods);
    ui_mod_free(render.ui.mod);
    ui_log_free(render.ui.log);
    ui_stars_free(render.ui.stars);
    ui_star_free(render.ui.star);
    ui_item_free(render.ui.item);
    ui_io_free(render.ui.io);
    factory_free(render.ui.factory);
    map_free(render.ui.map);

    ui_clipboard_free(&render.ui.board);
}

static void ui_event(SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_q)
            if (event->key.keysym.mod & KMOD_CTRL)
                render_push_quit();
    }

    if (ui_topbar_event(render.ui.topbar, event)) return;
    if (ui_status_event(render.ui.status, event)) return;
    if (ui_tapes_event(render.ui.tapes, event)) return;
    if (ui_mods_event(render.ui.mods, event)) return;
    if (ui_mod_event(render.ui.mod, event)) return;
    if (ui_log_event(render.ui.log, event)) return;
    if (ui_stars_event(render.ui.stars, event)) return;
    if (ui_star_event(render.ui.star, event)) return;
    if (ui_item_event(render.ui.item, event)) return;
    if (ui_io_event(render.ui.io, event)) return;
    if (factory_event(render.ui.factory, event)) return;
    if (map_event(render.ui.map, event)) return;
}

static void ui_render(SDL_Renderer *renderer)
{
    map_render(render.ui.map, renderer);
    factory_render(render.ui.factory, renderer);
    ui_topbar_render(render.ui.topbar, renderer);
    ui_status_render(render.ui.status, renderer);
    ui_tapes_render(render.ui.tapes, renderer);
    ui_mods_render(render.ui.mods, renderer);
    ui_mod_render(render.ui.mod, renderer);
    ui_log_render(render.ui.log, renderer);
    ui_stars_render(render.ui.stars, renderer);
    ui_star_render(render.ui.star, renderer);
    ui_item_render(render.ui.item, renderer);
    ui_io_render(render.ui.io, renderer);
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

void render_init(struct save_ring *in, struct save_ring *out)
{
    render.init = true;

    sdl_err(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    SDL_DisplayMode display = {0};
    sdl_err(SDL_GetDesktopDisplayMode(0, &display));
    render.rect = (SDL_Rect) { .x = 0, .y = 0, .w = display.w, .h = display.h };

    sdl_err(SDL_CreateWindowAndRenderer(
                    render.rect.w, render.rect.h,
                    SDL_WINDOW_BORDERLESS,
                    &render.window, &render.renderer));

    render.event = SDL_RegisterEvents(1);
    if (render.event == (uint32_t) -1) sdl_fail();

    render.focus = true;
    fonts_init(render.renderer);

    // We need the initial state from the sim before we can initialize our
    // UI. Not the greatest solution but it works so meh.
    render.out = out;
    render.proxy = proxy_new(in, out);
    while (!proxy_update(render.proxy));

    cursor_init();
    ui_init();
}

void render_close(void)
{
    proxy_free(render.proxy);

    ui_close();
    cursor_close();
    fonts_close();

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

    cursor_update();
    if (proxy_update(render.proxy))
        render_push_event(EV_STATE_UPDATE, 0, 0);

    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        cursor_event(&event);
        ui_event(&event);
    }

    save_ring_wake_signal(render.out);

    {
        sdl_err(SDL_RenderClear(render.renderer));

        ui_render(render.renderer);
        cursor_render(render.renderer);

        SDL_RenderPresent(render.renderer);
    }

    return true;
}

void render_loop(void)
{
    enum { fps_cap = 60 };
    ts_t sleep = ts_sec / fps_cap;

    ts_t ts = ts_now();
    while (render_step()) {
        ts = ts_sleep_until(ts + sleep);

        render.ticks++;
        render_push_event(EV_TICK, render.ticks, 0);
    }

    save_ring_close(render.out);
    save_ring_wake_signal(render.out);
}

void render_thread(void)
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

void render_push_quit(void)
{
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
        ui_status_set(render.ui.status, type, msg, len);
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
