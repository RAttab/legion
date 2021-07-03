/* core.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "core.h"

#include "render/font.h"
#include "render/sprites.h"
#include "render/map.h"
#include "render/ui.h"
#include "game/coord.h"
#include "game/world.h"
#include "game/atoms.h"
#include "game/prog.h"
#include "vm/mod.h"
#include "utils/log.h"
#include "utils/time.h"


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct core core = {0};

static void core_load_apply(void);
static void core_exec_event(enum event code, uint64_t d0, uint64_t d1);


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

static void state_init()
{
    enum { state_freq = 10 };
    core.state.sleep = ts_sec / state_freq;
    core.state.next = ts_now();

    core.state.world = world_new();
    core.state.home = world_populate(core.state.world);
}

static void state_close()
{
    world_free(core.state.world);
}

static void state_step(ts_t now)
{
    if (core.state.loading) { core_load_apply(); return; }
    if (now < core.state.next) return;

    world_step(core.state.world);
    core_push_event(EV_STATE_UPDATE, 0, 0);

    core.state.next += core.state.sleep;

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

static void ui_init(void)
{
    core.ui.map = map_new();
    core.ui.topbar = ui_topbar_new();
    core.ui.mods = ui_mods_new();
    core.ui.mod = ui_mod_new();
    core.ui.star = ui_star_new();
    core.ui.item = ui_item_new();
    core.ui.io = ui_io_new();
}

static void ui_close()
{
    ui_topbar_free(core.ui.topbar);
    ui_mods_free(core.ui.mods);
    ui_mod_free(core.ui.mod);
    ui_star_free(core.ui.star);
    ui_item_free(core.ui.item);
    ui_io_free(core.ui.io);
    map_free(core.ui.map);
}

static void ui_event(SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_q)
            if (event->key.keysym.mod & KMOD_CTRL)
                core_quit();
    }

    if (ui_topbar_event(core.ui.topbar, event)) return;
    if (ui_mods_event(core.ui.mods, event)) return;
    if (ui_mod_event(core.ui.mod, event)) return;
    if (ui_star_event(core.ui.star, event)) return;
    if (ui_item_event(core.ui.item, event)) return;
    if (ui_io_event(core.ui.io, event)) return;
    if (map_event(core.ui.map, event)) return;
}

static void ui_render(SDL_Renderer *renderer)
{
    map_render(core.ui.map, renderer);
    ui_topbar_render(core.ui.topbar, renderer);
    ui_mods_render(core.ui.mods, renderer);
    ui_mod_render(core.ui.mod, renderer);
    ui_star_render(core.ui.star, renderer);
    ui_item_render(core.ui.item, renderer);
    ui_io_render(core.ui.io, renderer);
}


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

void core_init()
{
    vm_atoms_init();
    atoms_register();
    vm_compile_init();
    prog_load();

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
        core_push_event(EV_TICK, core.ticks, 0);
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

static void core_exec_event(enum event code, uint64_t d0, uint64_t d1)
{
    SDL_Event ev = {0};
    ev.type = core.event;
    ev.user.code = code;
    ev.user.data1 = (void *) d0;
    ev.user.data2 = (void *) d1;

    ui_event(&ev);
}

// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

enum { core_save_version = 1 };

void core_save(void)
{
    struct save *save = save_new("./legion.save", core_save_version);
    world_save(core.state.world, save);

    size_t bytes = save_len(save);
    save_close(save);
    dbg("saved %zu bytes", bytes);
}

void core_load(void)
{
    if (core.state.loading) return;
    core.state.loading = true;
}

static void core_load_apply(void)
{
    assert(core.state.loading);

    struct save *save = save_load("./legion.save");
    assert(save_version(save) == core_save_version);

    struct world *world = world_load(save);
    size_t bytes = save_len(save);
    if (!world) goto done;

    struct world *old = core.state.world;
    core.state.world = world;
    core_exec_event(EV_STATE_LOAD, 0, 0);
    world_free(old);

  done:
    save_close(save);
    core.state.loading = false;
    dbg("loaded %zu bytes", bytes);
}
