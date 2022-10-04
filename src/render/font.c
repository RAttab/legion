/* font.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "font.h"
#include "game/sys.h"
#include "utils/bits.h"


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
struct font *font_mono10;

static struct font_repo { struct font *styles[font_style_max]; } fonts[font_size_max] = {0};

struct font *make_font(enum font_size size, enum font_style style)
{
    assert(size < font_size_max);
    assert(style < font_style_max);

    struct font *font = fonts[size].styles[style];
    assert(font);

    return font;
}


// -----------------------------------------------------------------------------
// title
// -----------------------------------------------------------------------------

FT_Library ft_library;

void fonts_populate(SDL_Renderer *renderer)
{
    ft_err(FT_Init_FreeType(&ft_library));

    void load_size(size_t index, uint8_t pt)
    {
        char path[PATH_MAX] = {0};
        struct font_repo *repo = fonts + index;

        sys_path_res("font/IbmPlexMonoRegular.otf", path, sizeof(path));
        repo->styles[font_nil] = font_open(renderer, path, pt);

        sys_path_res("font/IbmPlexMonoBold.otf", path, sizeof(path));
        repo->styles[font_bold] = font_open(renderer, path, pt);

        sys_path_res("font/IbmPlexMonoItalic.otf", path, sizeof(path));
        repo->styles[font_italic] = font_open(renderer, path, pt);

        assert(repo->styles[font_nil]->glyph_h == repo->styles[font_bold]->glyph_h);
        assert(repo->styles[font_nil]->glyph_w == repo->styles[font_bold]->glyph_w);
    }

    load_size(font_small, 6);
    load_size(font_big, 8);

    font_mono6 = make_font(font_small, font_nil);
    font_mono8 = make_font(font_big, font_nil);
}

void fonts_close()
{
    for (uint8_t pt = 0; pt < array_len(fonts); ++pt) {
        for (enum font_style st = 0; st < font_style_max; ++st) {
            struct font *font = fonts[pt].styles[st];
            if (font) font_close(font);
        }
    }
}

struct font *font_open(SDL_Renderer *renderer, const char *ttf, size_t pt)
{
    struct font *font = calloc(1, sizeof(*font));

    float hdpi = 0, vdpi = 0;
    sdl_err(SDL_GetDisplayDPI(0, NULL, &hdpi, &vdpi));

    FT_Face face;
    ft_err(FT_New_Face(ft_library, ttf, 0, &face));
    ft_err(FT_Set_Char_Size(face, 0, pt << 6, hdpi, vdpi));
    assert(FT_IS_SCALABLE(face));

    // The face level metric information is beyond my comprehension (bbox is in
    // units per EM and I have no idea how to conver that to pixels; pretty sure
    // I need to use the DPI...).
    //
    // instead go through every glyph and use the metrics information to figure
    // out what the actual bounding rectangle is for our font. Note that all
    // vertical metrics are against the baseline and not the bottom of the bbox.
    int64_t width = 0;
    int64_t ascender = 0;
    int64_t descender = 0;
    for (size_t i = 0; i < charmap_len; ++i) {
        ft_err(FT_Load_Char(face, charmap_start + i, FT_LOAD_RENDER));
        FT_Glyph_Metrics *metrics = &face->glyph->metrics;
        width = legion_max(width, metrics->width);
        ascender = legion_max(ascender, metrics->horiBearingY);
        descender = legion_max(descender, metrics->height - metrics->horiBearingY);
    }
    font->glyph_w = i64_ceil_div(width, 64);
    font->glyph_h = i64_ceil_div(ascender + descender, 64);
    font->glyph_baseline = i64_ceil_div(ascender, 64);

    size_t tex_w = font->glyph_w * charmap_len;
    font->tex = sdl_ptr(SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                    tex_w, font->glyph_h));

    for (size_t i = 0; i < charmap_len; ++i) {
        ft_err(FT_Load_Char(face, charmap_start + i, FT_LOAD_RENDER));
        FT_GlyphSlot slot = face->glyph;
        FT_Glyph_Metrics *metrics = &slot->metrics;

        FT_Bitmap *ft_bitmap = &slot->bitmap;
        assert(ft_bitmap->pitch == (int)ft_bitmap->width);
        assert(ft_bitmap->width <= font->glyph_w);
        assert(ft_bitmap->rows <= font->glyph_h);
        assert(ft_bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);
        assert(ft_bitmap->num_grays == 256);

        size_t pixels = ft_bitmap->rows * ft_bitmap->width;
        uint32_t sdl_bitmap[pixels];
        for (size_t px = 0; px < pixels; ++px)
            sdl_bitmap[px] = (ft_bitmap->buffer[px] * 0x01000000) | 0x00FFFFFF;

        // Just to confirm that I actually understand how this shit works.
        assert(metrics->horiBearingX >> 6 == slot->bitmap_left);

        SDL_Rect dst = {
            .x = (font->glyph_w * i) + slot->bitmap_left,
            .y = font->glyph_baseline - slot->bitmap_top,
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

void font_text_size(struct font *font, size_t len, size_t *w, size_t *h)
{
    if (w) *w = font->glyph_w * len;
    if (h) *h = font->glyph_h;
}

void font_render_bg(
        struct font *font, SDL_Renderer *renderer,
        SDL_Point pos, struct rgba fg, struct rgba bg,
        const char *str, size_t len)
{
    if (!rgba_is_nil(bg)) {
        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = pos.x,
                            .y = pos.y,
                            .w = font->glyph_w * len,
                            .h = font->glyph_h,
                        }));
    }

    sdl_err(SDL_SetTextureColorMod(font->tex, fg.r, fg.g, fg.b));
    sdl_err(SDL_SetTextureAlphaMod(font->tex, fg.a));
    sdl_err(SDL_SetTextureBlendMode(font->tex, SDL_BLENDMODE_BLEND));

    SDL_Rect src = { .x = 0, .y = 0, .w = font->glyph_w, .h = font->glyph_h };
    SDL_Rect dst = { .x = pos.x, .y = pos.y, .w = font->glyph_w, .h = font->glyph_h };

    for (size_t i = 0; i < len; ++i) {
        if (likely(str[i] >= charmap_start || str[i] < charmap_end)) {
            src.x = (str[i] - charmap_start) * src.w;
            sdl_err(SDL_RenderCopy(renderer, font->tex, &src, &dst));
        }

        dst.x += src.w;
    }
}

void font_render(
        struct font *font, SDL_Renderer *renderer,
        SDL_Point pos, struct rgba fg,
        const char *str, size_t len)
{
    font_render_bg(font, renderer, pos, fg, rgba_nil(), str, len);
}
