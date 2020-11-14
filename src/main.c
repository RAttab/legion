/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "SDL.h"

#include "utils.h"
#include "sector.h"
#include "font.h"
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

    sdl_err(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    sdl_err(SDL_GetDesktopDisplayMode(0, &display));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    sdl_err(SDL_CreateWindowAndRenderer(
                    display.w, display.h,
                    SDL_WINDOW_BORDERLESS, &window, &renderer));

    fonts_init(renderer);

    loop(renderer);

    fonts_close();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
