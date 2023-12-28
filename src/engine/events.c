/* events.c
   Remi Attab (remi.attab@gmail.com), 28 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

enum event_type : uint8_t
{
    ev_type_nil = 0,
    ev_type_key,
    ev_type_button,
    ev_type_scroll,
    ev_type_len,
};

enum event_state : uint8_t
{
    event_state_nil = 0,
    event_state_queued,
    event_state_armed,
};


struct event
{
    union
    {
        struct ev_key key;
        struct ev_button button;
        struct ev_scroll scroll;
    } data;

    enum event_type type;
};

// Required for the generic functions void pointer shenanigans to work properly.
static_assert(offsetof(struct event, data) == 0);


// unlikely but if someone rolls their face across the keyboard...?
constexpr size_t ev_queue_len = 64;

struct
{
    struct
    {
        bool update, in;
        struct ev_mouse ev;
        struct pos prev;
        enum ev_state buttons[3];
    } mouse;

    struct
    {
        size_t len, count[ev_type_len];
        struct event queue[ev_queue_len];
    } io;

    struct
    {
        struct { enum event_state state; struct ev_load ev; } load;
        struct { enum event_state state; struct ev_select_star ev; } select_star;
        struct { enum event_state state; struct ev_select_item ev; } select_item;
        struct { enum event_state state; struct ev_mod_break ev; } mod_break;
    } legion;
} events = {0};


// -----------------------------------------------------------------------------
// events - io
// -----------------------------------------------------------------------------

static const void *ev_next(const void *raw, enum event_type type)
{
    if (!events.io.count[type]) return nullptr;

    const struct event *it = raw;
    it = it ? it + 1 : events.io.queue;
    assert(it >= events.io.queue && it <= events.io.queue + events.io.len);

    for (; it < (events.io.queue + events.io.len); ++it)
        if (it->type == type) return it;

    return nullptr;
}

static void ev_consume(const void *raw, enum event_type type)
{
    // casting away the const feels icky but oh well.
    struct event *it = (void *) raw;

    assert(it->type == type);
    it->type = ev_type_nil;
    events.io.count[type]--;
}

static void *ev_push(enum event_type type)
{
    assert(events.io.len < ev_queue_len);

    struct event *ev = events.io.queue + events.io.len;
    memset(ev, 0, sizeof(*ev));
    ev->type = type;

    events.io.count[type]++;
    events.io.len++;
    return ev;
}


static void ev_key_fn(GLFWwindow *, int key, int, int action, int mods)
{
    if (key < 0) return;

    struct ev_key *ev = ev_push(ev_type_key);

    ev->c = key;
    if (ev->c < INT8_MAX && str_is_upper_case(ev->c))
        ev->c = str_lower_case(ev->c);

    switch (action)
    {
    case GLFW_RELEASE: { ev->state = ev_state_up; break; }
    case GLFW_PRESS: { ev->state = ev_state_down; break; }
    case GLFW_REPEAT: { ev->state = ev_state_repeat; break; }
    default: { assert(false); }
    }

    if (mods & GLFW_MOD_ALT) ev->mods = ev->mods | ev_mods_alt;
    if (mods & GLFW_MOD_CONTROL) ev->mods = ev->mods | ev_mods_ctrl;
    if (mods & GLFW_MOD_SHIFT) ev->mods = ev->mods | ev_mods_shift;
    if (mods & GLFW_MOD_CAPS_LOCK) ev->mods = ev->mods | ev_mods_caps_lock;
}

const struct ev_key *ev_next_key(const struct ev_key *ev)
{
    return ev_next(ev, ev_type_key);
}

void ev_consume_key(const struct ev_key *ev)
{
    ev_consume(ev, ev_type_key);
}


static size_t ev_button_index(uint8_t button)
{
    switch (button)
    {
    case ev_button_left: { return 0; }
    case ev_button_right: { return 1; }
    case ev_button_middle: { return 2; }
    default: { assert(false); }
    }
}

static void ev_button_fn(GLFWwindow *, int button, int action, int mods)
{
    struct ev_button *ev = ev_push(ev_type_button);

    ev->button = button;

    switch (action)
    {
    case GLFW_RELEASE: { ev->state = ev_state_up; break; }
    case GLFW_PRESS: { ev->state = ev_state_down; break; }
    case GLFW_REPEAT: { ev->state = ev_state_repeat; break; }
    default: { assert(false); }
    }

    if (mods & GLFW_MOD_ALT) ev->mods = ev->mods | ev_mods_alt;
    if (mods & GLFW_MOD_CONTROL) ev->mods = ev->mods | ev_mods_ctrl;
    if (mods & GLFW_MOD_SHIFT) ev->mods = ev->mods | ev_mods_shift;
    if (mods & GLFW_MOD_CAPS_LOCK) ev->mods = ev->mods | ev_mods_caps_lock;

    events.mouse.buttons[ev_button_index(ev->button)] = ev->state;
}

const struct ev_button *ev_next_button(const struct ev_button *ev)
{
    return ev_next(ev, ev_type_button);
}

enum ev_state ev_button_state(uint8_t button)
{
    return events.mouse.buttons[ev_button_index(button)];
}

bool ev_button_down(uint8_t button)
{
    return ev_button_state(button) == ev_state_down;
}

void ev_consume_button(const struct ev_button *ev)
{
    ev_consume(ev, ev_type_button);
}


static void ev_scroll_fn(GLFWwindow *, double xoff, double yoff)
{
    struct ev_scroll *ev = ev_push(ev_type_scroll);
    ev->dx = floor(xoff);
    ev->dy = floor(yoff);
}

const struct ev_scroll *ev_next_scroll(const struct ev_scroll *ev)
{
    return ev_next(ev, ev_type_scroll);
}

void ev_consume_scroll(const struct ev_scroll *ev)
{
    ev_consume(ev, ev_type_scroll);
}


static struct pos ev_mouse_convert(double x, double y)
{
    struct dim bounds = engine_viewport();
    return make_pos(
            legion_bound(floor(x), 0, bounds.w),
            legion_bound(floor(y), 0, bounds.h));

}

static void ev_mouse_move_fn(GLFWwindow *, double x, double y)
{
    struct ev_mouse *ev = &events.mouse.ev;
    ev->p = ev_mouse_convert(x, y);
}

static void ev_mouse_enter_fn(GLFWwindow *, int entered)
{
    events.mouse.in = entered;
}

const struct ev_mouse *ev_mouse(void)
{
    struct ev_mouse *ev = &events.mouse.ev;

    if (unlikely(events.mouse.update)) {
        events.mouse.update = false;
        ev->d.x = ev->p.x - events.mouse.prev.x;
        ev->d.y = ev->p.y - events.mouse.prev.y;
        events.mouse.prev = ev->p;
    }

    if (!events.mouse.in) return nullptr;
    if (!ev->d.x && !ev->d.y) return nullptr;
    return ev;
}

struct pos ev_mouse_pos(void)
{
    return events.mouse.ev.p;
}

bool ev_mouse_in(struct rect rect)
{
    return rect_contains(rect, events.mouse.ev.p);
}

static void ev_mouse_init(void)
{
    double x = 0.0f, y = 0.0f;
    glfwGetCursorPos(engine_window(), &x, &y);
    events.mouse.ev.p = events.mouse.prev = ev_mouse_convert(x, y);
}

// -----------------------------------------------------------------------------
// events - legion
// -----------------------------------------------------------------------------

const struct ev_load *ev_load(void)
{
    return events.legion.load.state == event_state_armed ?
        &events.legion.load.ev : nullptr;
}

void ev_set_load(void)
{
    dbgf("ev.queue: %p", &events.legion.load.state);
    events.legion.load.state = event_state_queued;
}


const struct ev_select_star *ev_select_star(void)
{
    return events.legion.select_star.state == event_state_armed ?
        &events.legion.select_star.ev : nullptr;
}

void ev_set_select_star(struct coord star)
{
    events.legion.select_star.state = event_state_queued;
    events.legion.select_star.ev = (struct ev_select_star) { .star = star };
}


const struct ev_select_item *ev_select_item(void)
{
    return events.legion.select_item.state == event_state_armed ?
        &events.legion.select_item.ev : nullptr;
}

void ev_set_select_item(struct coord star, im_id item)
{
    events.legion.select_item.state = event_state_queued;
    events.legion.select_item.ev = (struct ev_select_item) {
        .star = star, .item = item,
    };
}

const struct ev_mod_break *ev_mod_break(void)
{
    return events.legion.mod_break.state == event_state_armed ?
        &events.legion.mod_break.ev : nullptr;
}

void ev_set_mod_break(mod_id mod, vm_ip ip)
{
    events.legion.mod_break.state = event_state_queued;
    events.legion.mod_break.ev = (struct ev_mod_break) {
        .mod = mod, .ip = ip,
    };
}


// -----------------------------------------------------------------------------
// setup
// -----------------------------------------------------------------------------

static void events_init(void)
{
    GLFWwindow *win = engine_window();
    glfwSetKeyCallback(win, &ev_key_fn);
    glfwSetMouseButtonCallback(win, &ev_button_fn);
    glfwSetScrollCallback(win, &ev_scroll_fn);
    glfwSetCursorPosCallback(win, &ev_mouse_move_fn);
    glfwSetCursorEnterCallback(win, &ev_mouse_enter_fn);

    ev_mouse_init();
}

static void events_close(void) {}

void events_poll(void)
{
    events.mouse.update = true;
    memset(&events.io, 0, sizeof(events.io));

    void update_state(enum event_state *state)
    {
        switch (*state)
        {
        case event_state_nil: { break; }
        case event_state_queued: { *state = event_state_armed; break; }
        case event_state_armed: { *state = event_state_nil; break; }
        default: { assert(false); }
        }
    }
    update_state(&events.legion.load.state);
    update_state(&events.legion.select_star.state);
    update_state(&events.legion.select_item.state);
    update_state(&events.legion.mod_break.state);

    glfwPollEvents();
}
