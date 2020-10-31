#include "SDL.h"

SDL_DisplayMode display = {0};

bool process(SDL_Event *event)
{
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
    if (SDL_CreateWindowAndRenderer(dm.w, dm.h, SDL_WINDOW_BORDERLESS, &window, &renderer) != 0) {
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
