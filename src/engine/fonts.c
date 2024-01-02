/* fonts.c
   Remi Attab (remi.attab@gmail.com), 30 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

constexpr char fonts_map_first = 33;
constexpr char fonts_map_last = 127;
constexpr size_t fonts_map_len = (fonts_map_last - fonts_map_first) + 1;

struct font_glyph { unit w, h, baseline; };
struct { GLuint id; struct font_glyph glyph; } fonts[font_len];

static GLuint font_id(enum render_font font)
{
    assert(font && font < font_len);

    GLuint id = fonts[font].id;
    assert(id);

    return id;
}

static struct font_glyph font_glyph(enum render_font font)
{
    assert(font && font < font_len);
    return fonts[font].glyph;
}

static float fonts_depth(char c)
{
    return likely(c >= fonts_map_first && c < fonts_map_last) ?
        (c - fonts_map_first + 1) : 0.0f;
}


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

struct font
{
    const char *name;

    const void *data;
    size_t len;

    uint32_t pt;
    struct font_glyph glyph;
    uint32_t *texels;
};

static struct font fonts_db(enum render_font id)
{
    switch (id)
    {

    case font_base: {
        extern const uint8_t db_font_regular_data[];
        extern const uint32_t db_font_regular_len;
        return (struct font) {
            .name = "base",
            .data = db_font_regular_data,
            .len = db_font_regular_len,
        };
    }

    case font_bold: {
        extern const uint8_t db_font_bold_data[];
        extern const uint32_t db_font_bold_len;
        return (struct font) {
            .name = "bold",
            .data = db_font_bold_data,
            .len = db_font_bold_len,
        };
    }

    case font_italic: {
        extern const uint8_t db_font_italic_data[];
        extern const uint32_t db_font_italic_len;
        return (struct font) {
            .name = "italic",
            .data = db_font_italic_data,
            .len = db_font_italic_len,
        };
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ft_library;

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

static uint32_t fonts_face_pt(FT_Face face, struct dpi dpi)
{
    // Empirical tests indicates that % is the one character that consistently
    // gives us the width of the face in all 3 of our font types.
    unit face_width(uint32_t pt)
    {
        ft_err(FT_Set_Char_Size(face, 0, pt, dpi.h, dpi.v));
        ft_err(FT_Load_Char(face, '%', FT_LOAD_RENDER));
        return face->glyph->metrics.width;
    }

    const uint32_t one = 1U << 6;
    const unit target = (engine_viewport().w / engine_cols_len) * 64;

    unit width = 0;
    uint32_t pt = one;
    for (; (width = face_width(pt)) < target; pt += one);
    if (width == target) return pt;

    for (uint32_t first = pt - one, last = pt; first < last; ) {
        pt = (last - first) / 2;
        width = face_width(pt);
        if (width == target) return pt;
        if (width < target) { last = pt; } else { first = pt; }
    }

    return pt;
}

static void fonts_freetype(struct font *font)
{
    FT_Open_Args args = {
        .flags = FT_OPEN_MEMORY,
        .memory_base = font->data,
        .memory_size = font->len,
    };

    FT_Face face;
    ft_err(FT_Open_Face(ft_library, &args, 0, &face));
    assert(FT_IS_SCALABLE(face));

    struct dpi dpi = engine_dpi();
    const uint32_t pt = fonts_face_pt(face, dpi);
    ft_err(FT_Set_Char_Size(face, 0, pt, dpi.h, dpi.v));

    int64_t width = 0;
    int64_t ascender = 0;
    int64_t descender = 0;
    for (size_t i = 0; i < fonts_map_len; ++i) {
        ft_err(FT_Load_Char(face, fonts_map_first + i, FT_LOAD_RENDER));
        FT_Glyph_Metrics *metrics = &face->glyph->metrics;
        width = legion_max(width, metrics->width);
        ascender = legion_max(ascender, metrics->horiBearingY);
        descender = legion_max(descender, metrics->height - metrics->horiBearingY);
    }

    struct font_glyph glyph = {
        .w = i64_ceil_div(width, 64),
        .h = i64_ceil_div(ascender + descender, 64),
        .baseline = i64_ceil_div(ascender, 64),
    };

    const size_t glyph_len = glyph.w * glyph.h;
    uint32_t *texels = calloc(glyph_len * fonts_map_len, sizeof(*texels));

    // First glyph is blank to deal with all the non-printable characters.
    for (size_t i = 1; i < fonts_map_len; ++i) {
        ft_err(FT_Load_Char(face, fonts_map_first + i - 1, FT_LOAD_RENDER));
        FT_GlyphSlot slot = face->glyph;
        FT_Glyph_Metrics *metrics = &slot->metrics;

        FT_Bitmap *ft_bitmap = &slot->bitmap;
        assert(ft_bitmap->pitch == (int)ft_bitmap->width);
        assert(ft_bitmap->width <= glyph.w);
        assert(ft_bitmap->rows <= glyph.h);
        assert(ft_bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);
        assert(ft_bitmap->num_grays == 256);

        // Just to confirm that I actually understand how this shit works.
        assert(metrics->horiBearingX >> 6 == slot->bitmap_left);

        uint32_t *out = texels + (glyph_len * i);
        for (size_t y = 0; y < ft_bitmap->rows; ++y) {
            for (size_t x = 0; x < ft_bitmap->width; ++x) {
                size_t col = x + slot->bitmap_left;
                size_t row = glyph.baseline - slot->bitmap_top + y;
                uint8_t in = ft_bitmap->buffer[y * ft_bitmap->width + x];
                out[row * glyph.w + col] = 0xFFFFFF00 | in;
            }
        }
    }

    FT_Done_Face(face);

    font->pt = pt;
    font->glyph = glyph;
    font->texels = texels;
}

#undef ft_error

static void fonts_load(enum render_font type)
{
    struct font font = fonts_db(type);
    fonts_freetype(&font);

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);

    glTexImage3D(
            GL_TEXTURE_2D_ARRAY, 0,
            GL_RGBA, font.glyph.w, font.glyph.h, fonts_map_len, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, font.texels);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    free(font.texels);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    fonts[type].id = id;
    fonts[type].glyph = font.glyph;
}

static struct font_glyph fonts_populate(void)
{
    ft_err(FT_Init_FreeType(&ft_library));

    for (enum render_font id = 1; id < font_len; ++id)
        fonts_load(id);

    return fonts[font_base].glyph;
}

static void fonts_close(void)
{
    for (enum render_font id = 1; id < font_len; ++id)
        glDeleteTextures(1, &fonts[id].id);
}
