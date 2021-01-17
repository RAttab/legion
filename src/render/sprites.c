/* sprites.c
   Rémi Attab (remi.attab@gmail.com), 17 Jan 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "sprites.h"


// -----------------------------------------------------------------------------
// sprites
// -----------------------------------------------------------------------------

struct sprites *sprites_items = NULL;

void sprites_init(SDL_Renderer *renderer)
{
    char path[PATH_MAX] = {0};

    core_path_res("items.bmp", path, sizeof(path));
    sprites_items = sprites_open(renderer, path, 50, 50);
}


struct sprites *sprites_open(
        SDL_Renderer *renderer, const char *path, size_t w, size_t h)
{
    SDL_Surface *surface = sdl_ptr(SDL_LoadBMP(path));
    SDL_Texture *tex = sdl_ptr(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_FreeSurface(surface);

    int total_w = 0, total_h = 0;
    sdl_err(SDL_QueryTexture(tex, NULL, NULL, &total_w, &total_h));
    assert(total_w % w == 0 && total_h % h == 0);

    struct sprites *sprites = calloc(1, sizeof(*sprites));
    *sprites = (struct sprites) {
        .w = w,
        .h = h,
        .rows = total_w / w,
        .cols = total_h / h,
        .tex = tex,
    };

    sprites->len = sprites->rows * sprites->cols;
    return sprites;
}

void sprites_close(struct sprites *sprites)
{
    SDL_DestroyTexture(sprites->tex);
    free(sprites);
}

void sprites_reset(struct sprites *sprites)
{
    sdl_err(SDL_SetTextureAlphaMod(sprites->tex, 0xFF));
    sdl_err(SDL_SetTextureColorMod(sprites->tex, 0xFF, 0xFF, 0xFF));
    sdl_err(SDL_SetTextureBlendMode(sprites->tex, SDL_BLENDMODE_BLEND));
}

void sprites_render(
        struct sprites *sprites, SDL_Renderer *renderer, size_t index, SDL_Rect *dst)
{
    assert(index < sprites->len);

    SDL_Rect src = {
        .x = (index % sprites->rows) * sprites->w,
        .y = (index / sprites->rows) * sprites->h,
        .w = sprites->w, .h = sprites->h,
    };
    sdl_err(SDL_RenderCopy(renderer, sprites->tex, &src, dst));
}
