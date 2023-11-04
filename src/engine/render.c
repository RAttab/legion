/* render.c
   Remi Attab (remi.attab@gmail.com), 01 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// buffer
// -----------------------------------------------------------------------------

constexpr render_layer render_layer_cap = 32;

struct legion_packed vertex_prim { float pos[4]; float fg[4]; };
struct legion_packed vertex_tex  { float pos[4]; float fg[4]; float tex[2]; };
struct legion_packed vertex_font { float pos[4]; float fg[4]; float tex[3]; float bg[4]; };

union vertex_ptr
{
    void *raw;
    struct vertex_prim *prim;
    struct vertex_tex *tex;
    struct vertex_font *font;
};

enum render_type : unsigned
{
    render_type_nil = 0,

    render_type_prim_line,
    render_type_prim_tri,
    render_type_tex_star,
    render_type_font_base,
    render_type_font_bold,

    render_type_len,

    render_type_tex_cursor,
};


struct render_buffer
{
    enum render_type type;

    GLuint vao, vbo, ebo, shader, texture;

    size_t vlen, vcap, vertex_len;
    union vertex_ptr v;

    size_t elen, ecap;
    uint32_t *e;
};

static void render_buffer_init(
        struct render_buffer *buffer, enum render_type type)
{
    if (likely(buffer->type == type)) return;

    assert(!buffer->type);
    buffer->type = type;

    glGenVertexArrays(1, &buffer->vao);
    glBindVertexArray(buffer->vao);

    glGenBuffers(1, &buffer->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);

    glGenBuffers(1, &buffer->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->ebo);

    switch (type)
    {
    case render_type_prim_line:
    case render_type_prim_tri: {
        buffer->shader = shader_id(shader_prim);
        buffer->texture = 0;
        break;
    }

    case render_type_tex_star: {
        buffer->shader = shader_id(shader_tex);
        buffer->texture = texture_id(texture_star);
        break;
    }
    case render_type_tex_cursor: {
        buffer->shader = shader_id(shader_tex);
        buffer->texture = texture_id(texture_cursor);
        break;
    }

    case render_type_font_base: {
        buffer->shader = shader_id(shader_font);
        buffer->texture = font_id(font_base);
        break;
    }
    case render_type_font_bold: {
        buffer->shader = shader_id(shader_font);
        buffer->texture = font_id(font_bold);
        break;
    }

    default: { assert(false); }
    }

    switch (type)
    {

    case render_type_prim_line:
    case render_type_prim_tri: {
        struct vertex_prim *verts = buffer->v.prim;
        buffer->vertex_len = sizeof(*verts);

        glVertexAttribPointer(
                0,
                array_len(verts[0].pos), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_prim, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
                1,
                array_len(verts[0].fg), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_prim, fg));
        glEnableVertexAttribArray(1);

        break;
    }

    case render_type_tex_star:
    case render_type_tex_cursor: {
        struct vertex_tex *verts = buffer->v.tex;
        buffer->vertex_len = sizeof(*verts);

        glVertexAttribPointer(
                0,
                array_len(verts[0].pos), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_tex, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
                1,
                array_len(verts[0].fg), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_tex, fg));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
                2,
                array_len(verts[0].tex), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_tex, tex));
        glEnableVertexAttribArray(2);

        break;
    }

    case render_type_font_base:
    case render_type_font_bold: {
        struct vertex_font *verts = buffer->v.font;
        buffer->vertex_len = sizeof(*verts);

        glVertexAttribPointer(
                0,
                array_len(verts[0].pos), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_font, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
                1,
                array_len(verts[0].fg), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_font, fg));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
                2,
                array_len(verts[0].tex), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_font, tex));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(
                3,
                array_len(verts[0].bg), GL_FLOAT, GL_FALSE,
                sizeof(verts[0]), (void *) offsetof(struct vertex_font, bg));
        glEnableVertexAttribArray(3);

        break;
    }

    default: { assert(false); }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void render_buffer_close(struct render_buffer *buffer)
{
    free(buffer->e);
    free(buffer->v.raw);

    glDeleteBuffers(1, &buffer->ebo);
    glDeleteBuffers(1, &buffer->vbo);
    glDeleteVertexArrays(1, &buffer->vao);
}

static void render_buffer_reserve_vertex(struct render_buffer *buffer, size_t len)
{
    if (likely(buffer->vlen + len <= buffer->vcap)) return;

    size_t old = buffer->vcap;
    if (!buffer->vcap) buffer->vcap = 128;
    while (buffer->vcap < buffer->vlen + len) buffer->vcap *= 2;

    buffer->v.raw = realloc_zero(buffer->v.raw, old, buffer->vcap, buffer->vertex_len);
}

static union vertex_ptr render_buffer_push_vertex(struct render_buffer *buffer)
{
    assert(buffer->type);
    assert(buffer->vlen < buffer->vcap);

    union vertex_ptr ptr = {0};
    switch (buffer->type)
    {
    case render_type_prim_line:
    case render_type_prim_tri: { ptr.prim = buffer->v.prim + buffer->vlen; break; }
    case render_type_tex_star:
    case render_type_tex_cursor: { ptr.tex = buffer->v.tex + buffer->vlen; break; }
    case render_type_font_base:
    case render_type_font_bold: { ptr.font = buffer->v.font + buffer->vlen; break; }
    default: { assert(false); }
    }

    buffer->vlen++;
    return ptr;
}

static void render_buffer_reserve_index(struct render_buffer *buffer, size_t len)
{
    if (likely(buffer->elen + len <= buffer->ecap)) return;

    size_t old = buffer->ecap;
    if (!buffer->ecap) buffer->ecap = 128;
    while (buffer->ecap < buffer->elen + len) buffer->ecap *= 2;

    buffer->e = realloc_zero(buffer->e, old, buffer->ecap, sizeof(*buffer->e));
}

static void render_buffer_push_index(
        struct render_buffer *buffer, union vertex_ptr ptr)
{
    assert(buffer->type);
    assert(buffer->elen < buffer->ecap);

    assert(ptr.raw >= buffer->v.raw);
    assert(ptr.raw < buffer->v.raw + buffer->vlen * buffer->vertex_len);

    ptrdiff_t index = 0;
    switch (buffer->type)
    {
    case render_type_prim_line:
    case render_type_prim_tri: { index = ptr.prim - buffer->v.prim; break; }
    case render_type_tex_star:
    case render_type_tex_cursor: { index = ptr.tex - buffer->v.tex; break; }
    case render_type_font_base:
    case render_type_font_bold: { index = ptr.font - buffer->v.font; break; }
    default: { assert(false); }
    }

    assert(index >= 0 && index < UINT32_MAX);
    buffer->e[buffer->elen] = index;

    assert(buffer->elen < UINT32_MAX);
    buffer->elen++;
}

static void render_buffer_clear(struct render_buffer *buffer)
{
    buffer->vlen = buffer->elen = 0;
}

static void render_buffer_draw(struct render_buffer *buffer)
{
    if (!buffer->type || !buffer->vlen) return;

    glBindVertexArray(buffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer->vlen * buffer->vertex_len, buffer->v.raw, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer->elen * sizeof(*buffer->e), buffer->e, GL_DYNAMIC_DRAW);

    GLenum draw = GL_TRIANGLES, texture = 0;
    switch (buffer->type)
    {
    case render_type_prim_line: { draw = GL_LINES; break; }
    case render_type_tex_star:
    case render_type_tex_cursor: { texture = GL_TEXTURE_2D; break; }
    case render_type_font_base:
    case render_type_font_bold: { texture = GL_TEXTURE_2D_ARRAY; break; }
    default: { break; }
    }

    glUseProgram(buffer->shader);
    if (texture) glBindTexture(texture, buffer->texture);

    glDrawElements(draw, buffer->elen, GL_UNSIGNED_INT, 0);
    static_assert(sizeof(*buffer->e) == 4); // You need to adjust the type above

    if (texture) glBindTexture(texture, 0);
    glUseProgram(0);

    glBindVertexArray(0);
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static struct
{
    struct
    {
        render_layer top, max;
        render_layer stack[render_layer_cap];
        size_t len;
    } layers;

    struct render_buffer cursor;
    struct render_buffer buffer[render_layer_cap][render_type_len];
} render;


static void render_init(void)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    render_buffer_init(&render.cursor, render_type_tex_cursor);
}

static void render_close(void)
{
    for (render_layer layer = 0; layer < render.layers.max; ++layer)
        for (enum render_type type = 0; type < render_type_len; ++type)
            render_buffer_close(&render.buffer[layer][type]);
    render_buffer_close(&render.cursor);
}

render_layer render_layer_push(render_layer n)
{
    assert(n + render.layers.top < render_layer_cap);
    assert(render.layers.len + 1 < render_layer_cap);

    render_layer prev = render.layers.top;
    render.layers.top += n;
    render.layers.max = legion_max(render.layers.max, render.layers.top);

    render.layers.stack[render.layers.len] = prev;
    render.layers.len++;

    return prev;
}

render_layer render_layer_push_max()
{
    render_layer max = render.layers.max;
    return max + render_layer_push(max);
}

void render_layer_pop(void)
{
    assert(render.layers.len);

    render.layers.len--;
    render.layers.top = render.layers.stack[render.layers.len];
}

void render_clear(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (render_layer layer = 0; layer < render.layers.max; ++layer)
        for (enum render_type type = 0; type < render_type_len; ++type)
            render_buffer_clear(&render.buffer[layer][type]);
    render_buffer_clear(&render.cursor);

    render.layers.top = render.layers.max = render.layers.len = 0;
}

void render_draw(void)
{
    if (render.layers.len)
        errf("unbalanced layer stack: %zu", render.layers.len);

    for (render_layer layer = 0; layer < render.layers.max; ++layer)
        for (enum render_type type = 0; type < render_type_len; ++type)
            render_buffer_draw(&render.buffer[layer][type]);
    render_buffer_draw(&render.cursor);

    glfwSwapBuffers(engine_window());
}

static struct render_buffer *render_buffer(
        render_layer layer, enum render_type type)
{
    struct render_buffer *buffer = &render.cursor;

    if (likely(type != render_type_tex_cursor)) {
        assert(layer < render.layers.top);
        buffer = &render.buffer[layer][type];
    }

    render_buffer_init(buffer, type);
    return buffer;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

// We do our projection math with doubles because some of the coordinate systems
// won't fit in floats leading to inaccurate results. Once everything's been
// normalized to the [-1, +1] range, we can truncate to float without any
// signifcant loss of precision.
static inline float render_project_ndc(double v, double o, double n)
{
    return (((v - o) * 2) / n) - 1.0f;
}

#define render_project_pos(layer, pos, area)                \
    {                                                       \
        render_project_ndc(pos.x, area.x, area.w),          \
        -render_project_ndc(pos.y, area.y, area.h),         \
        -(((double) layer) / render_layer_cap / 2.0f),      \
        1.0f,                                               \
    }

#define render_project_rgba(rgba)               \
    {                                           \
        ((float) rgba.r) / UINT8_MAX,           \
        ((float) rgba.g) / UINT8_MAX,           \
        ((float) rgba.b) / UINT8_MAX,           \
        ((float) rgba.a) / UINT8_MAX,           \
    }

void render_line(
        render_layer layer, struct rgba fg, struct line line)
{
    render_line_a(layer, fg, line, engine_area());
}

void render_line_a(
        render_layer layer, struct rgba fg, struct line line, struct rect area)
{
    struct render_buffer *buffer = render_buffer(layer, render_type_prim_line);
    render_buffer_reserve_vertex(buffer, 2);
    render_buffer_reserve_index(buffer, 2);

    union vertex_ptr va = render_buffer_push_vertex(buffer);
    *va.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, line.a, area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vb = render_buffer_push_vertex(buffer);
    *vb.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, line.b, area),
        .fg = render_project_rgba(fg),
    };

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vb);
}

void render_triangle(
        render_layer layer, struct rgba fg, struct triangle tri)
{
    render_triangle_a(layer, fg, tri, engine_area());
}

void render_triangle_a(
        render_layer layer, struct rgba fg, struct triangle tri, struct rect area)
{
    struct render_buffer *buffer = render_buffer(layer, render_type_prim_tri);
    render_buffer_reserve_vertex(buffer, 3);
    render_buffer_reserve_index(buffer, 3);

    union vertex_ptr va = render_buffer_push_vertex(buffer);
    *va.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, tri.a, area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vb = render_buffer_push_vertex(buffer);
    *vb.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, tri.b, area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vc = render_buffer_push_vertex(buffer);
    *vc.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, tri.c, area),
        .fg = render_project_rgba(fg),
    };

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vb);
    render_buffer_push_index(buffer, vc);
}


void render_rect_line(
        render_layer layer, struct rgba fg, struct rect r)
{
    render_rect_line_a(layer, fg, r, engine_area());
}

void render_rect_line_a(
        render_layer layer, struct rgba fg, struct rect r, struct rect area)
{
    struct render_buffer *buffer = render_buffer(layer, render_type_prim_line);
    render_buffer_reserve_vertex(buffer, 4);
    render_buffer_reserve_index(buffer, 8);

    union vertex_ptr va = render_buffer_push_vertex(buffer);
    *va.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vb = render_buffer_push_vertex(buffer);
    *vb.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vc = render_buffer_push_vertex(buffer);
    *vc.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y + r.h), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vd = render_buffer_push_vertex(buffer);
    *vd.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y + r.h), area),
        .fg = render_project_rgba(fg),
    };

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vb);

    render_buffer_push_index(buffer, vb);
    render_buffer_push_index(buffer, vc);

    render_buffer_push_index(buffer, vc);
    render_buffer_push_index(buffer, vd);

    render_buffer_push_index(buffer, vd);
    render_buffer_push_index(buffer, va);
}

void render_rect_fill(
        render_layer layer, struct rgba fg, struct rect r)
{
    render_rect_fill_a(layer, fg, r, engine_area());
}

void render_rect_fill_a(
        render_layer layer, struct rgba fg, struct rect r, struct rect area)
{
    struct render_buffer *buffer = render_buffer(layer, render_type_prim_tri);
    render_buffer_reserve_vertex(buffer, 4);
    render_buffer_reserve_index(buffer, 6);

    union vertex_ptr va = render_buffer_push_vertex(buffer);
    *va.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vb = render_buffer_push_vertex(buffer);
    *vb.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vc = render_buffer_push_vertex(buffer);
    *vc.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y + r.h), area),
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vd = render_buffer_push_vertex(buffer);
    *vd.prim = (struct vertex_prim) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y + r.h), area),
        .fg = render_project_rgba(fg),
    };

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vb);
    render_buffer_push_index(buffer, vd);

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vc);
    render_buffer_push_index(buffer, vd);
}

static void render_tex_a(
        render_layer layer,
        enum render_type type,
        struct rgba fg,
        struct rect r,
        struct rect area)
{
    struct render_buffer *buffer = render_buffer(layer, type);
    render_buffer_reserve_vertex(buffer, 4);
    render_buffer_reserve_index(buffer, 6);

    union vertex_ptr va = render_buffer_push_vertex(buffer);
    *va.tex = (struct vertex_tex) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y), area),
        .tex = { 0.0f, 1.0f },
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vb = render_buffer_push_vertex(buffer);
    *vb.tex = (struct vertex_tex) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y), area),
        .tex = { 1.0f, 1.0f },
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vc = render_buffer_push_vertex(buffer);
    *vc.tex = (struct vertex_tex) {
        .pos = render_project_pos(layer, make_pos(r.x, r.y + r.h), area),
        .tex = { 0.0f, 0.0f },
        .fg = render_project_rgba(fg),
    };

    union vertex_ptr vd = render_buffer_push_vertex(buffer);
    *vd.tex = (struct vertex_tex) {
        .pos = render_project_pos(layer, make_pos(r.x + r.w, r.y + r.h), area),
        .tex = { 1.0f, 0.0f },
        .fg = render_project_rgba(fg),
    };

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vb);
    render_buffer_push_index(buffer, vd);

    render_buffer_push_index(buffer, va);
    render_buffer_push_index(buffer, vc);
    render_buffer_push_index(buffer, vd);
}

unit render_cursor_size(void)
{
    return engine_area().w / 96;
}

static void render_cursor(struct pos p)
{
    unit size = render_cursor_size();
    struct rect r = make_rect(p.x, p.y, size, size);
    render_tex_a(render_layer_cap, render_type_tex_cursor, rgba_white(), r, engine_area());
}

void render_star(
        render_layer layer,
        struct rgba fg,
        struct rect r,
        struct rect area)
{
    render_tex_a(layer, render_type_tex_star, fg, r, area);
}

void render_font(
        render_layer layer,
        enum render_font font,
        struct rgba fg,
        struct pos pos,
        const char *str, size_t len)
{
    render_font_bg_a(layer, font, fg, rgba_nil(), pos, engine_area(), str, len);
}

void render_font_a(
        render_layer layer,
        enum render_font font,
        struct rgba fg,
        struct pos pos,
        struct rect area,
        const char *str, size_t len)
{
    render_font_bg_a(layer, font, fg, rgba_nil(), pos, area, str, len);
}

void render_font_bg(
        render_layer layer,
        enum render_font font,
        struct rgba fg, struct rgba bg,
        struct pos pos,
        const char *str, size_t len)
{
    render_font_bg_a(layer, font, fg, bg, pos, engine_area(), str, len);
}

void render_font_bg_a(
        render_layer layer,
        enum render_font font,
        struct rgba fg, struct rgba bg,
        struct pos pos,
        struct rect area,
        const char *str, size_t len)
{
    enum render_type type = render_type_nil;
    switch (font)
    {
    case font_base: { type = render_type_font_base; break; }
    case font_bold: { type = render_type_font_bold; break; }
    default: { assert(false); }
    }

    struct render_buffer *buffer = render_buffer(layer, type);
    render_buffer_reserve_vertex(buffer, 4 * len);
    render_buffer_reserve_index(buffer, 6 * len);

    struct pos it = pos;
    struct font_glyph g = font_glyph(font);

    for (size_t i = 0; i < len; ++i, it.x += g.w) {
        if (unlikely(str[i] == '\n')) {
            it = make_pos(pos.x, it.y + g.h);
            continue;
        }

        float d = fonts_depth(str[i]);
        if (d == 0.0f) continue;

        union vertex_ptr va = render_buffer_push_vertex(buffer);
        *va.font = (struct vertex_font) {
            .pos = render_project_pos(layer, make_pos(it.x, it.y), area),
            .tex = { 0.0f, 0.0f, d },
            .fg = render_project_rgba(fg),
            .bg = render_project_rgba(bg),
        };

        union vertex_ptr vb = render_buffer_push_vertex(buffer);
        *vb.font = (struct vertex_font) {
            .pos = render_project_pos(layer, make_pos(it.x + g.w, it.y), area),
            .tex = { 1.0f, 0.0f, d },
            .fg = render_project_rgba(fg),
            .bg = render_project_rgba(bg),
        };

        union vertex_ptr vc = render_buffer_push_vertex(buffer);
        *vc.font = (struct vertex_font) {
            .pos = render_project_pos(layer, make_pos(it.x, it.y + g.h), area),
            .tex = { 0.0f, 1.0f, d },
            .fg = render_project_rgba(fg),
            .bg = render_project_rgba(bg),
        };

        union vertex_ptr vd = render_buffer_push_vertex(buffer);
        *vd.font = (struct vertex_font) {
            .pos = render_project_pos(layer, make_pos(it.x + g.w, it.y + g.h), area),
            .tex = { 1.0f, 1.0f, d },
            .fg = render_project_rgba(fg),
            .bg = render_project_rgba(bg),
        };

        render_buffer_push_index(buffer, va);
        render_buffer_push_index(buffer, vb);
        render_buffer_push_index(buffer, vd);

        render_buffer_push_index(buffer, va);
        render_buffer_push_index(buffer, vc);
        render_buffer_push_index(buffer, vd);
    }
}
