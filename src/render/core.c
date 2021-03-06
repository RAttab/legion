/* core.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "core.h"

#include "render/font.h"
#include "render/sprites.h"
#include "render/map.h"
#include "render/panel.h"
#include "game/coord.h"
#include "game/galaxy.h"
#include "game/atoms.h"
#include "vm/mod.h"
#include "utils/log.h"
#include "utils/time.h"


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct core core = {0};


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

static void state_init()
{
    enum { state_freq = 10 };
    core.state.sleep = ts_sec / state_freq;
    core.state.next = ts_now();

    core.state.sector = sector_gen((struct coord) { .x = 0x0101, .y = 0x0101 });
    sector_preload(core.state.sector);
}

static void state_close() {}

static void state_step(ts_t now)
{
    if (now < core.state.next) return;

    sector_step(core.state.sector);
    core_push_event(EV_STATE_UPDATE, 0, 0);

    core.state.next += core.state.sleep;
    core.state.time++;

    if (core.state.next <= now) {
        core.state.next = now + core.state.sleep;
        dbg("sim.late> now:%lu, next:%lu, sleep:%lu, ticks:%lu",
                now, core.state.next, core.state.sleep, core.ticks);
    }
}


// -----------------------------------------------------------------------------
// cursor
// -----------------------------------------------------------------------------

static void cursor_init()
{
    core.cursor.size = 20;
    core.cursor.point = (SDL_Point){
        .x = core.rect.w / 2,
        .y = core.rect.h / 2
    };

    char path[PATH_MAX];
    core_path_res("cursor.bmp", path, sizeof(path));

    SDL_Surface *surface = sdl_ptr(SDL_LoadBMP(path));
    core.cursor.tex = sdl_ptr(SDL_CreateTextureFromSurface(core.renderer, surface));
    SDL_FreeSurface(surface);

    sdl_err(SDL_SetRelativeMouseMode(true));
    sdl_err(SDL_SetTextureBlendMode(core.cursor.tex, SDL_BLENDMODE_ADD));
    sdl_err(SDL_SetTextureColorMod(core.cursor.tex, 0xFF, 0xFF, 0xFF));
}

static void cursor_close()
{
    SDL_DestroyTexture(core.cursor.tex);
}

static void cursor_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_MOUSEMOTION: {
        core.cursor.point.x += event->motion.xrel;
        core.cursor.point.y += event->motion.yrel;
        break;
    }
    }
}

static void cursor_render(SDL_Renderer *renderer)
{
    sdl_err(SDL_RenderCopy(renderer, core.cursor.tex,
                    &(SDL_Rect){ .x = 0, .y = 0, .w = 50, .h = 50 },
                    &(SDL_Rect){
                        .x = core.cursor.point.x,
                        .y = core.cursor.point.y,
                        .w = core.cursor.size,
                        .h = core.cursor.size }));
}


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

static void ui_init()
{
    core.ui.map = map_new();
    core.ui.mods = panel_mods_new();
    core.ui.code = panel_code_new();
    core.ui.star = panel_star_new();
    core.ui.obj = panel_obj_new();
    core.ui.menu = panel_menu_new();
    core.ui.pos = panel_pos_new();
}

static void ui_close()
{
    map_free(core.ui.map);
    panel_free(core.ui.menu);
    panel_free(core.ui.mods);
    panel_free(core.ui.code);
    panel_free(core.ui.pos);
    panel_free(core.ui.star);
    panel_free(core.ui.obj);
}

static void ui_event(SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_q)
            if (event->key.keysym.mod & KMOD_CTRL)
                core_quit();
    }

    if (panel_event(core.ui.menu, event)) return;
    if (panel_event(core.ui.mods, event)) return;
    if (panel_event(core.ui.code, event)) return;
    if (panel_event(core.ui.pos, event)) return;
    if (panel_event(core.ui.star, event)) return;
    if (panel_event(core.ui.obj, event)) return;
    if (map_event(core.ui.map, event)) return;
}

static void ui_render(SDL_Renderer *renderer)
{
    map_render(core.ui.map, renderer);
    panel_render(core.ui.menu, renderer);
    panel_render(core.ui.mods, renderer);
    panel_render(core.ui.code, renderer);
    panel_render(core.ui.pos, renderer);
    panel_render(core.ui.star, renderer);
    panel_render(core.ui.obj, renderer);
}


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

void core_init()
{
    vm_atoms_init();
    atoms_register();
    vm_compile_init();
    mods_preload();

    sdl_err(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    SDL_DisplayMode display = {0};
    sdl_err(SDL_GetDesktopDisplayMode(0, &display));
    core.rect = (SDL_Rect) { .x = 0, .y = 0, .w = display.w, .h = display.h };

    sdl_err(SDL_CreateWindowAndRenderer(
                    core.rect.w, core.rect.h,
                    SDL_WINDOW_BORDERLESS,
                    &core.window, &core.renderer));

    core.event = SDL_RegisterEvents(1);
    if (core.event == (uint32_t) -1) sdl_fail();

    fonts_init(core.renderer);
    sprites_init(core.renderer);
    state_init();
    cursor_init();
    ui_init();
}

void core_close()
{
    ui_close();
    cursor_close();
    state_close();
    fonts_close();

    SDL_DestroyRenderer(core.renderer);
    SDL_DestroyWindow(core.window);
    SDL_Quit();

    mods_free();
}

void core_path_res(const char *name, char *dst, size_t len)
{
    snprintf(dst, len, "./res/%s", name);
}

void core_run()
{
    enum { fps_cap = 60 };
    ts_t sleep = ts_sec / fps_cap;

    ts_t ts = ts_now();
    while (true) {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return;
            cursor_event(&event);
            ui_event(&event);
        }

        sdl_err(SDL_RenderClear(core.renderer));
        ui_render(core.renderer);
        cursor_render(core.renderer);
        SDL_RenderPresent(core.renderer);

        state_step(ts);
        ts = ts_sleep_until(ts + sleep);
        core.ticks++;
    }
}

void core_quit()
{
    sdl_err(SDL_PushEvent(&(SDL_Event){ .type = SDL_QUIT }));
}

void core_push_event(enum event code, uint64_t d0, uint64_t d1)
{
    SDL_Event ev = {0};
    ev.type = core.event;
    ev.user.code = code;
    ev.user.data1 = (void *) d0;
    ev.user.data2 = (void *) d1;

    int ret = sdl_err(SDL_PushEvent(&ev));
    assert(ret > 0);
}
