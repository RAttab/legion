/* core.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "core.h"

#include "font.h"
#include "map.h"
#include "panel.h"
#include "sector.h"


struct core core = {0};


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

static void state_init()
{
    core.state.sector = sector_gen((struct coord) {0});
}

static void state_close() {}


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
    SDL_RenderCopy(renderer, core.cursor.tex,
            &(SDL_Rect){ .x = 0, .y = 0, .w = 50, .h = 50 },
            &(SDL_Rect){
                .x = core.cursor.point.x,
                .y = core.cursor.point.y,
                .w = core.cursor.size,
                .h = core.cursor.size });
}


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

static void ui_init()
{
    core.ui.map = map_new();
    core.ui.pos = panel_pos_new();
    core.ui.system = panel_system_new();
}

static void ui_close()
{
    panel_free(core.ui.pos);
    panel_free(core.ui.system);
    map_free(core.ui.map);
}

static void ui_event(SDL_Event *event)
{
    if (panel_event(core.ui.pos, event)) return;
    if (panel_event(core.ui.system, event)) return;
    if (map_event(core.ui.map, event)) return;
}

static void ui_render(SDL_Renderer *renderer)
{
    map_render(core.ui.map, renderer);
    panel_render(core.ui.pos, renderer);
    panel_render(core.ui.system, renderer);
}


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

void core_init()
{
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

    bool quit = false;
    while (!quit) {

        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { fprintf(stderr, "quit\n"); return; }
            cursor_event(&event);
            ui_event(&event);
        }

        sdl_err(SDL_RenderClear(core.renderer));
        ui_render(core.renderer);
        cursor_render(core.renderer);
        SDL_RenderPresent(core.renderer);
    }
}

void core_quit()
{
    sdl_err(SDL_PushEvent(&(SDL_Event){ .type = SDL_QUIT }));
}

void core_push_event(enum event code, void *data)
{
    SDL_Event ev = {0};
    ev.type = core.event;
    ev.user.code = code;
    ev.user.data1 = data;
    
    int ret = sdl_err(SDL_PushEvent(&ev));
    assert(ret > 0);
}
