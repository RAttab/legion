/* font.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "font.h"
#include "render/core.h"


// -----------------------------------------------------------------------------
// freetype
// -----------------------------------------------------------------------------

#include <ft2build.h>
#include FT_FREETYPE_H


#define ft_err(p)                                                       \
    ({                                                                  \
        typeof(p) err = (p);                                            \
        if (unlikely(err)) {                                            \
            fprintf(stderr, "ft-error<%s, %u> %s: %s (%d)\n",           \
                    __FILE__, __LINE__, #p, FT_Error_String(err), err); \
            abort();                                                    \
        }                                                               \
        err;                                                            \
    })

// -----------------------------------------------------------------------------
// pre-loaded-fonts
// -----------------------------------------------------------------------------

struct font *font_mono4;
struct font *font_mono6;
struct font *font_mono8;


// -----------------------------------------------------------------------------
// title
// -----------------------------------------------------------------------------

FT_Library ft_library;

void fonts_init(SDL_Renderer *renderer)
{
    ft_err(FT_Init_FreeType(&ft_library));

    char path[PATH_MAX];
    core_path_res("GeneraleStationGX.ttf", path, sizeof(path));

    font_mono4 = font_open(renderer, path, 4);
    font_mono8 = font_open(renderer, path, 8);
}

void fonts_close()
{
    font_close(font_mono8);
}

struct font *font_open(SDL_Renderer *renderer, const char *ttf, size_t pt)
{
    struct font *font = calloc(1, sizeof(*font));

    float dpi_w = 0, dpi_h = 0;
    sdl_err(SDL_GetDisplayDPI(0, &dpi_w, &dpi_h, NULL));

    FT_Face face;
    ft_err(FT_New_Face(ft_library, ttf, 0, &face));
    ft_err(FT_Set_Char_Size(face, 0, pt*64, dpi_w, dpi_h));

    FT_Size_Metrics *metrics = &face->size->metrics;
    font->glyph_w = metrics->x_ppem;
    font->glyph_h = (metrics->ascender - metrics->descender) >> 6;
    font->glyph_baseline = metrics->ascender >> 6;

    size_t tex_w = font->glyph_w * charmap_len;
    font->tex = sdl_ptr(SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                    tex_w, font->glyph_h));

    for (size_t i = 0; i < charmap_len; ++i) {
        ft_err(FT_Load_Char(face, charmap_start + i, FT_LOAD_RENDER));

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap *ft_bitmap = &slot->bitmap;
        assert(ft_bitmap->pitch == (int)ft_bitmap->width);

        size_t pixels = ft_bitmap->rows * ft_bitmap->width;
        uint32_t sdl_bitmap[pixels];
        for (size_t px = 0; px < pixels; ++px) {
            sdl_bitmap[px] = 0xFF000000 | (ft_bitmap->buffer[px] * 0x00010101);
        }

        SDL_Rect dst = {
            .x = (font->glyph_w * i) + slot->bitmap_left,
            // Note: the -1 is required otherwise the bitmap can be off-by one
            // in height with a max-level descender. The ascender also never
            // touches the top if not given. Pretty sure it has to do with the
            // change in coordinate system and what 0 means.
            .y = font->glyph_baseline - slot->bitmap_top - 1,
            .w = ft_bitmap->width,
            .h = ft_bitmap->rows,
        };
        assert((size_t)(dst.w) <= font->glyph_w);
        assert((size_t)(dst.x+dst.w) <= tex_w);
        assert((size_t)(dst.y+dst.h) <= font->glyph_h);

        size_t pitch = dst.w * sizeof(sdl_bitmap[0]);
        sdl_err(SDL_UpdateTexture(font->tex, &dst, sdl_bitmap, pitch));
    }

    FT_Done_Face(face);
    return font;
}

void font_close(struct font *font)
{
    SDL_DestroyTexture(font->tex);
    free(font);
}

void font_reset(struct font *font)
{
    sdl_err(SDL_SetTextureAlphaMod(font->tex, 0xFF));
    sdl_err(SDL_SetTextureColorMod(font->tex, 0xFF, 0xFF, 0xFF));
    sdl_err(SDL_SetTextureBlendMode(font->tex, SDL_BLENDMODE_ADD));
}

void font_text_size(struct font *font, size_t len, size_t *w, size_t *h)
{
    *w = font->glyph_w * len;
    *h = font->glyph_h;
}

void font_render(
        struct font *font,
        SDL_Renderer *renderer,
        const char *str,
        size_t len,
        SDL_Point pos)
{
    SDL_Rect src = { .x = 0, .y = 0, .w = font->glyph_w, .h = font->glyph_h };
    SDL_Rect dst = { .x = pos.x, .y = pos.y, .w = font->glyph_w, .h = font->glyph_h };

    for (size_t i = 0; i < len; ++i) {

        if (str[i] == '\n') {
            dst.x = pos.x;
            dst.y += src.h;
        }

        if (likely(str[i] >= charmap_start || str[i] < charmap_end)) {
            src.x = (str[i] - charmap_start) * src.w;
            sdl_err(SDL_RenderCopy(renderer, font->tex, &src, &dst));
        }

        dst.x += src.w;
    }
}
