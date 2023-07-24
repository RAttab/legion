/* img.c
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/res.h"
#include "utils/err.h"


// -----------------------------------------------------------------------------
// img
// -----------------------------------------------------------------------------

static SDL_Texture *db_img_load_bmp(
        SDL_Renderer *renderer, const void *data, size_t len)
{
    SDL_RWops *stream = sdl_ptr(SDL_RWFromConstMem(data, len));
    SDL_Surface *surface = sdl_ptr(SDL_LoadBMP_RW(stream, 1));
    SDL_Texture *tex = sdl_ptr(SDL_CreateTextureFromSurface(renderer, surface));
    SDL_FreeSurface(surface);
    return tex;
}

SDL_Texture *db_img_cursor(SDL_Renderer *renderer)
{
    extern const uint8_t db_img_cursor_data[];
    extern const uint32_t db_img_cursor_len;
    return db_img_load_bmp(renderer, db_img_cursor_data, db_img_cursor_len);
}

SDL_Texture *db_img_map(SDL_Renderer *renderer)
{
    extern const uint8_t db_img_map_data[];
    extern const uint32_t db_img_map_len;
    return db_img_load_bmp(renderer, db_img_map_data, db_img_map_len);
}


// -----------------------------------------------------------------------------
// font
// -----------------------------------------------------------------------------

struct db_font db_font_regular(void)
{
    extern const uint8_t db_font_regular_data[];
    extern const uint32_t db_font_regular_len;
    return (struct db_font) {
        .len = db_font_regular_len,
        .ptr = db_font_regular_data,
    };
}

struct db_font db_font_italic(void)
{
    extern const uint8_t db_font_italic_data[];
    extern const uint32_t db_font_italic_len;
    return (struct db_font) {
        .len = db_font_italic_len,
        .ptr = db_font_italic_data,
    };
}

struct db_font db_font_bold(void)
{
    extern const uint8_t db_font_bold_data[];
    extern const uint32_t db_font_bold_len;
    return (struct db_font) {
        .len = db_font_bold_len,
        .ptr = db_font_bold_data,
    };
}


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct legion_packed db_man_header
{
    uint8_t magic;
    uint32_t path_len;
    uint32_t data_len;
};

bool db_man_next(struct db_man_it *it)
{
    extern const uint8_t db_man_list[];
    extern const uint8_t db_man_list_end[];

    if (!it->end) {
        *it = (struct db_man_it) {
            .it = db_man_list,
            .end = db_man_list_end
        };
    }
    if (it->it >= it->end) return false;

    struct db_man_header header = *((const struct db_man_header *) it->it);
    assert(header.magic == 0xFF);
    it->it += sizeof(header);

    it->path = it->it;
    it->path_len = header.path_len;
    it->it += header.path_len;

    it->data = it->it;
    it->data_len = header.data_len;
    it->it += header.data_len;

    assert(it->it <= it->end);
    return true;
}
