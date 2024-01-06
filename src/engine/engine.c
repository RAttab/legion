/* engine.c
   Remi Attab (remi.attab@gmail.com), 23 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------


struct
{
    bool init;

    struct rect area;
    struct font_glyph glyph;

    struct dim viewport;
    GLFWmonitor *monitor;
    GLFWwindow *window;

    threads_id thread;
    struct threads *threads;

    char mods_path[PATH_MAX];
} engine = {0};


// -----------------------------------------------------------------------------
// engine
// -----------------------------------------------------------------------------

#define glfw_fail(p)                                            \
    do {                                                        \
        const char *msg = nullptr;                              \
        int code = glfwGetError(&msg);                          \
        if (!msg) msg = "nil";                                  \
        fprintf(stderr, "%s:%u: <err> %s: %s (%d)\n",           \
                __FILE__, __LINE__, #p, msg, code);             \
        abort();                                                \
    } while(false)


static void glfw_error(int code, const char *msg)
{
    failf("[glfw] %s (%d)", msg, code);
}

static GLAPIENTRY void gl_error(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei len,
        const GLchar *msg,
        const void *)
{
    const char *source_str = nullptr;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API: { source_str = "api"; break; }
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: { source_str = "window"; break; }
    case GL_DEBUG_SOURCE_SHADER_COMPILER: { source_str = "shader"; break; }
    case GL_DEBUG_SOURCE_THIRD_PARTY: { source_str = "3rd-party"; break; }
    case GL_DEBUG_SOURCE_APPLICATION: { source_str = "app"; break; }
    case GL_DEBUG_SOURCE_OTHER: { source_str = "other"; break; }
    default: { source_str = "nil"; break; }
    }

    const char *type_str = nullptr;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: { type_str = "error"; break; }
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: { type_str = "deprecated"; break; }
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: { type_str = "undefined"; break; }
    case GL_DEBUG_TYPE_PORTABILITY: { type_str = "port"; break; }
    case GL_DEBUG_TYPE_PERFORMANCE: { type_str = "perf"; break; }
    case GL_DEBUG_TYPE_MARKER: { type_str = "marker"; break; }
    case GL_DEBUG_TYPE_PUSH_GROUP: { type_str = "push"; break; }
    case GL_DEBUG_TYPE_POP_GROUP: { type_str = "pop"; break; }
    case GL_DEBUG_TYPE_OTHER: { type_str = "other"; break; }
    default: { type_str = "nil"; break; }
    }

    enum { dbg, err, fail } out = dbg;
    const char *severity_str = nullptr;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         { severity_str = "high"; out = fail; break; }
    case GL_DEBUG_SEVERITY_MEDIUM:       { severity_str = "med"; out = err; break; }
    case GL_DEBUG_SEVERITY_LOW:          { severity_str = "low"; out = err; break; }
    case GL_DEBUG_SEVERITY_NOTIFICATION: { severity_str = "info"; out = dbg; break; }
    default: { severity_str = "nil"; break; }
    }

    unsigned msg_len = len - 1;
    switch (out)
    {
    case dbg:  { dbgf( "[gl:%s:%s:%s] %.*s (%x)", source_str, type_str, severity_str, msg_len, msg, id); break; }
    case err:  { errf( "[gl:%s:%s:%s] %.*s (%x)", source_str, type_str, severity_str, msg_len, msg, id); break; }
    case fail: { failf("[gl:%s:%s:%s] %.*s (%x)", source_str, type_str, severity_str, msg_len, msg, id); break; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// engine
// -----------------------------------------------------------------------------

void engine_init(void)
{
    if (!glfwInit()) glfw_fail("glfwInit");
    glfwSetErrorCallback(&glfw_error);

    engine.monitor = glfwGetPrimaryMonitor();
    if (!engine.monitor) glfw_fail("glfwGetPrimaryMonitor");

    const GLFWvidmode *mode = glfwGetVideoMode(engine.monitor);
    if (!mode) glfw_fail("glfwGetVideoMode");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_SAMPLES, 4);

    engine.window = glfwCreateWindow(mode->width, mode->height, "Legion", engine.monitor, NULL);
    if (!engine.window) glfw_fail("glfwCreateWindow");

    glfwMakeContextCurrent(engine.window);
    glfwSwapInterval(0); // 1 for vsync

    gladLoadGL();

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&gl_error, nullptr);

    int width = 0, height = 0;
    glfwGetFramebufferSize(engine.window, &width, &height);
    engine.viewport = make_dim(width, height);
    glViewport(0, 0, width, height);

    glfwSetInputMode(engine.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(engine.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    events_init();
    shaders_populate();
    textures_populate();
    engine.glyph = fonts_populate();
    render_init();
    ux_init();
    sound_fork();

    engine.area.x = engine.area.y = 0;
    engine.area.w = engine.glyph.w * engine_cols_len;
    engine.area.h = (engine.area.w * engine.viewport.h) / engine.viewport.w;

    engine.init = true;
}

void engine_close(void)
{
    sound_join();
    ux_free();
    render_close();
    fonts_close();
    textures_close();
    shaders_close();
    events_close();

    glfwDestroyWindow(engine.window);
    glfwTerminate();
}

bool engine_initialized(void)
{
    return engine.init;
}


struct rect engine_area(void)
{
    return engine.area;
}

struct dim engine_cell(void)
{
    return make_dim(
            engine.glyph.w,
            engine.glyph.h);
}

unit engine_cell_baseline(void)
{
    return engine.glyph.baseline;
}

struct dim engine_dim(unit rows, unit cols)
{
    return make_dim(
            rows * engine.glyph.w,
            cols * engine.glyph.h);
}

struct dim engine_dim_margin(unit rows, unit cols, struct dim margin)
{
    return make_dim(
            (rows * engine.glyph.w) + (margin.w * 2),
            (cols * engine.glyph.h) + (margin.h * 2));
}

struct dim engine_viewport(void)
{
    return engine.viewport;
}

struct dpi engine_dpi(void)
{
    int hmm = 0, vmm = 0;
    glfwGetMonitorPhysicalSize(engine.monitor, &hmm, &vmm);
    const GLFWvidmode* mode = glfwGetVideoMode(engine.monitor);

    constexpr float factor = 25.4f; // mm to inches conversion factor.
    return (struct dpi) {
        .h = (mode->width * factor) / hmm,
        .v = (mode->height * factor) / vmm,
    };
}

GLFWwindow *engine_window(void)
{
    return engine.window;
}

// -----------------------------------------------------------------------------
// loop
// -----------------------------------------------------------------------------

static bool engine_step(void)
{
    if (engine.threads && threads_done(engine.threads, engine.thread))
        return false;

    switch (proxy_update())
    {
    case proxy_nil: { break; }
    case proxy_loaded: { ux_reset(); ev_set_load(); } // fallthrough
    case proxy_updated: { ux_update(); break; }
    default: { assert(false); }
    }

    events_poll();
    if (glfwWindowShouldClose(engine_window())) return false;
    ux_event();

    render_clear();
    {
        ux_render();
        render_cursor(ev_mouse_pos());
    }
    render_draw();

    return true;
}

void engine_loop(void)
{
    constexpr sys_ts period = sys_sec / engine_frame_rate;

    sys_ts ts = sys_now();
    while (engine_step()) ts = sys_sleep_until(ts + period);
}

bool engine_done(void)
{
    return threads_done(engine.threads, engine.thread);
}

void engine_fork(void)
{
    void engine_run(void *) { engine_loop(); }
    engine.threads = threads_alloc(threads_pool_engine);
    engine.thread = threads_fork(engine.threads, engine_run, nullptr);
}

void engine_join(void)
{
    threads_join(engine.threads, engine.thread);
    threads_free(engine.threads);
}

void engine_quit(void)
{
    glfwSetWindowShouldClose(engine.window, GLFW_TRUE);
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static void engine_populate_impl(void)
{
    im_populate();
    mod_compiler_init();
    specs_populate();
    tapes_populate();
    stars_populate();
    ast_populate();

    {
        struct atoms *atoms = atoms_new();

        im_populate_atoms(atoms);
        io_populate_atoms(atoms);
        man_populate(atoms);

        atoms_free(atoms);
    }
}

void engine_populate(void)
{
    snprintf(engine.mods_path, sizeof(engine.mods_path),
            "%s/.legion", path_home());

    int ret = mkdir(engine.mods_path, 0740);
    if (ret && errno != EEXIST)
        failf_errno("unable to create mods dir '%s'", engine.mods_path);

    engine_populate_impl();
}

void engine_populate_tests(void)
{
    threads_init(threads_profile_local);
    snprintf(engine.mods_path, sizeof(engine.mods_path), "./res/mods");

    engine_populate_impl();
}


// -----------------------------------------------------------------------------
// paths
// -----------------------------------------------------------------------------

void engine_path_mods(char *dst, size_t len)
{
    snprintf(dst, len, "%s", engine.mods_path);
}

void engine_path_mod(const char *name, char *dst, size_t len)
{
    snprintf(dst, len, "%s/%s.lisp", engine.mods_path, name);
}
