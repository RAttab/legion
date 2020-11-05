/* main.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "SDL.h"

#include <stdbool.h>

SDL_DisplayMode display = {0};

void process(SDL_Event *event)
{
    switch (event->type)
    {
        
    case SDL_KEYUP:
        switch (event->key.keysym.sym)
        {
        case SDLK_q:
            SDL_PushEvent(&(SDL_Event){.type = SDL_QUIT });
            break;
        }
        break;
    }
}

void render(SDL_Renderer *renderer)
{
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &(SDL_Rect){0, 0, display.w, display.h});

    SDL_RenderPresent(renderer);
}

void loop(SDL_Renderer *renderer)
{
    bool quit = false;
    while (!quit) {

        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return;
            process(&event);
        }

        render(renderer);
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
