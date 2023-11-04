/* shaders.c
   Remi Attab (remi.attab@gmail.com), 24 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

enum render_shader : unsigned
{
    shader_nil = 0,
    shader_prim,
    shader_tex,
    shader_font,
    shader_len,
};

static GLuint shaders[shader_len];
static GLuint shader_id(enum render_shader shader)
{
    assert(shader && shader < shader_len);

    GLuint id = shaders[shader];
    assert(id);

    return id;
}

// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

struct shader
{
    const char *name;

    GLuint id;
    GLenum type;
    GLint len;
    const GLchar *src;
};

static_assert(sizeof(GLchar) == sizeof(char));

static const char *shaders_db(
        enum render_shader shader, struct shader *vert, struct shader *frag)
{
    switch (shader)
    {

    case shader_prim: {

        vert->name = "prim.vert";
        vert->type = GL_VERTEX_SHADER;
        extern const uint32_t db_shader_prim_vert_len;
        vert->len = db_shader_prim_vert_len;
        extern const char db_shader_prim_vert_data[];
        vert->src = db_shader_prim_vert_data;

        frag->name = "prim.frag";
        frag->type = GL_FRAGMENT_SHADER;
        extern const uint32_t db_shader_prim_frag_len;
        frag->len = db_shader_prim_frag_len;
        extern const char db_shader_prim_frag_data[];
        frag->src = db_shader_prim_frag_data;

        return "prim";
    }

    case shader_tex: {

        vert->name = "tex.vert";
        vert->type = GL_VERTEX_SHADER;
        extern const uint32_t db_shader_tex_vert_len;
        vert->len = db_shader_tex_vert_len;
        extern const char db_shader_tex_vert_data[];
        vert->src = db_shader_tex_vert_data;

        frag->name = "tex.frag";
        frag->type = GL_FRAGMENT_SHADER;
        extern const uint32_t db_shader_tex_frag_len;
        frag->len = db_shader_tex_frag_len;
        extern const char db_shader_tex_frag_data[];
        frag->src = db_shader_tex_frag_data;

        return "tex";
    }

    case shader_font: {

        vert->name = "font.vert";
        vert->type = GL_VERTEX_SHADER;
        extern const uint32_t db_shader_font_vert_len;
        vert->len = db_shader_font_vert_len;
        extern const char db_shader_font_vert_data[];
        vert->src = db_shader_font_vert_data;

        frag->name = "font.frag";
        frag->type = GL_FRAGMENT_SHADER;
        extern const uint32_t db_shader_font_frag_len;
        frag->len = db_shader_font_frag_len;
        extern const char db_shader_font_frag_data[];
        frag->src = db_shader_font_frag_data;

        return "font";
    }

    default: { assert(false); }
    }
}

static void shaders_compile_shader(GLuint program, struct shader *shader)
{
    shader->id = glCreateShader(shader->type);

    glShaderSource(shader->id, 1, &shader->src, &shader->len);
    glCompileShader(shader->id);

    GLint status = GL_FALSE;
    glGetShaderiv(shader->id, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint log_len = 0;
        GLchar log[1024] = {0};
        glGetShaderInfoLog(shader->id, sizeof(log), &log_len, log);
        failf("[gl:shader:%s] %.*s", shader->name, (unsigned) log_len, log);
    }

    glAttachShader(program, shader->id);
}

static void shaders_compile_program(enum render_shader shader)
{
    struct shader vert = {0}, frag = {0};
    const char *name = shaders_db(shader, &vert, &frag);

    GLuint program = glCreateProgram();
    shaders_compile_shader(program, &vert);
    shaders_compile_shader(program, &frag);
    glLinkProgram(program);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint log_len = 0;
        GLchar log[1024] = {0};
        glGetProgramInfoLog(program, sizeof(log), &log_len, log);
        failf("[gl:shader:%s] %.*s", name, (unsigned) log_len, log);
    }

    shaders[shader] = program;
    glDeleteShader(vert.id);
    glDeleteShader(frag.id);
}

static void shaders_populate(void)
{
    for (enum render_shader shader = 1; shader < shader_len; ++shader)
        shaders_compile_program(shader);
}

static void shaders_close(void)
{
    for (enum render_shader shader = 1; shader < shader_len; ++shader)
        glDeleteProgram(shaders[shader]);
}
