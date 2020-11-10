/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "SDL.h"

#include "utils.h"
#include "sector.h"
#include "ui.h"

SDL_DisplayMode display = {0};

struct state
{
    struct sector *sector;
    struct ui_core *core;
};

void loop(SDL_Renderer *renderer)
{
    struct sector *sector = sector_gen((struct coord) {0});
    struct state state = {
        .sector = sector,
        .core = ui_core_init(
                renderer, sector,
                &(SDL_Rect){ .x = 0, .y = 0, .w = display.w, .h = display.h }),
    };

    bool quit = false;
    while (!quit) {

        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return;
            ui_core_events(state.core, &event);
        }

        SDL_RenderClear(renderer);
        ui_core_render(state.core, renderer);
        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char **argv)
{
    (void) argc; (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("failed to init: %s", SDL_GetError());
        goto fail_init;
    }

    if (SDL_GetDesktopDisplayMode(0, &display) != 0) {
        SDL_Log("failed to get resolution: %s", SDL_GetError());
        goto fail_dm;
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (SDL_CreateWindowAndRenderer(display.w, display.h, SDL_WINDOW_BORDERLESS, &window, &renderer) != 0) {
        SDL_Log("failed to create window: %s", SDL_GetError());
        goto fail_win;
    }

    loop(renderer);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

    SDL_DestroyWindow(window);
  fail_win:
  fail_dm:
    SDL_Quit();
  fail_init:
    return 1;
}
