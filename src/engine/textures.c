/* texture.c
   Remi Attab (remi.attab@gmail.com), 26 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

enum render_texture : unsigned
{
    texture_nil = 0,
    texture_cursor,
    texture_star,
    texture_len,
};

static GLuint textures[texture_len] = {0};
static GLuint texture_id(enum render_texture texture)
{
    assert(texture && texture < texture_len);

    GLuint id = textures[texture];
    assert(id);

    return id;
}

// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

struct texture_data
{
    const char *name;

    uint32_t len;
    const void *const bytes;

    unit width, height;
    uint32_t *img;
};

static struct texture_data textures_db(enum render_texture texture)
{
    switch (texture)
    {

    case texture_cursor: {
        extern const uint8_t db_texture_cursor_data[];
        extern const uint32_t db_texture_cursor_len;
        return (struct texture_data) {
            .name = "cursor.tga",
            .len = db_texture_cursor_len,
            .bytes = db_texture_cursor_data,
        };
    }

    case texture_star: {
        extern const uint8_t db_texture_star_data[];
        extern const uint32_t db_texture_star_len;
        return (struct texture_data) {
            .name = "star.tga",
            .len = db_texture_star_len,
            .bytes = db_texture_star_data,
        };
        break;
    }

    default: { assert(false); }
    }
}

// Reference: https://www.fileformat.info/format/tga/egff.htm
static void textures_tga(struct texture_data *data)
{
    const void *tga = data->bytes;
    const void *tga_end = tga + data->len;

    const struct legion_packed
    {
        uint8_t id_len;
        uint8_t map_type;
        uint8_t img_type;
        struct legion_packed { uint16_t first, len; uint8_t bits; } map;
        struct legion_packed { uint16_t x, y, w, h; uint8_t bits, desc; } img;
    } *head = tga;
    tga += sizeof(*head); assert(tga < tga_end);
    tga += head->id_len;

    if (head->map_type)
        failf("unsupported tga color map for '%s'", data->name);
    if (head->img_type != 10)
        failf("unsupported tga image type for '%s'", data->name);
    if (head->img.bits != 32)
        failf("unsupported tga image bits for '%s'", data->name);
    if (head->img.bits != 32)
        failf("unsupported tga image bits for '%s'", data->name);
    if (head->img.x || head->img.y)
        failf("unsupported tga image origin for '%s'", data->name);

    data->width = head->img.w;
    data->height = head->img.h;

    size_t len = head->img.w * head->img.h;
    data->img = mem_array_alloc_t(*data->img, len);

    uint32_t *out = data->img;
    uint32_t *const end = out + len;

    constexpr uint8_t rle_mask = 0x80U;
    uint32_t to_rgba(uint32_t c) { return (c << 8) | (c >> 24); }

    while (out < end) {
        const uint8_t *h = tga; tga += sizeof(*h);
        uint8_t n = (*h & ~rle_mask) + 1;

        if ((*h & rle_mask)) {
            const uint32_t *c = tga; tga += sizeof(*c);
            for (size_t i = 0; i < n; ++i, ++out) *out = to_rgba(*c);
            continue;
        }

        for (size_t i = 0; i < n; ++i, ++out) {
            const uint32_t *c = tga; tga += sizeof(*c);
            *out = to_rgba(*c);
        }
    }

    assert(tga <= tga_end);
    assert(out == end);
}

static void textures_load(enum render_texture texture)
{
    struct texture_data data = textures_db(texture);

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    textures_tga(&data);
    glTexImage2D(
            GL_TEXTURE_2D, 0,
            GL_RGBA, data.width, data.height, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data.img);
    glGenerateMipmap(GL_TEXTURE_2D);
    mem_free(data.img); data.img = nullptr;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    textures[texture] = id;
}

static void textures_populate(void)
{
    for (enum render_texture texture = 1; texture < texture_len; ++texture)
        textures_load(texture);
}

static void textures_close(void)
{
    for (enum render_texture texture = 1; texture < texture_len; ++texture)
        glDeleteTextures(1, textures + texture);
}
