/* img.c
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/img.h"
#include "utils/err.h"

// -----------------------------------------------------------------------------
// img
// -----------------------------------------------------------------------------

static SDL_Texture *img_load_bmp(
        SDL_Renderer *renderer, const void *data, size_t len)
{
    SDL_RWops *stream = sdl_ptr(SDL_RWFromConstMem(data, len));
    SDL_Surface *surface = sdl_ptr(SDL_LoadBMP_RW(stream, 1));
    SDL_Texture *tex = sdl_ptr(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_FreeSurface(surface);
    return tex;
}

SDL_Texture *img_cursor(SDL_Renderer *renderer)
{
    extern const uint8_t db_img_cursor_data[];
    extern const uint32_t db_img_cursor_len;
    return img_load_bmp(renderer, db_img_cursor_data, db_img_cursor_len);
}

SDL_Texture *img_map(SDL_Renderer *renderer)
{
    extern const uint8_t db_img_map_data[];
    extern const uint32_t db_img_map_len;
    return img_load_bmp(renderer, db_img_map_data, db_img_map_len);
}
