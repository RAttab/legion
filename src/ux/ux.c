/* ui.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "ux/ux.h"

#include <stdarg.h>

// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

const char *ux_slot_str(enum ux_slot slot)
{
    switch (slot)
    {
    case ux_slot_nil:       { return "nil"; }
    case ux_slot_back:      { return "back"; }
    case ux_slot_right:     { return "right"; }
    case ux_slot_right_sub: { return "rsub"; }
    case ux_slot_left:      { return "left"; }
    default:                { return "?"; }
    }
}

const char *ux_view_str(enum ux_view view)
{
    switch (view)
    {
    case ux_view_nil:     { return "nil"; }

    case ux_view_topbar:  { return "topbar"; }
    case ux_view_status:  { return "status"; }

    case ux_view_map:     { return "map"; }
    case ux_view_factory: { return "factory"; }

    case ux_view_tapes:   { return "tapes"; }
    case ux_view_stars:   { return "stars"; }
    case ux_view_mods:    { return "mods"; }
    case ux_view_log:     { return "log"; }

    case ux_view_star:    { return "star"; }
    case ux_view_item:    { return "item"; }
    case ux_view_pills:   { return "pills"; }
    case ux_view_energy:  { return "energy"; }
    case ux_view_workers: { return "workers"; }

    case ux_view_man:     { return "man"; }

    default:              { return "?"; }
    }
}



// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

struct
{
    bool init;
    enum ux_view slots[ux_slot_len];
    struct ux_view_state views[ux_view_len];
} ux = { .init = false };


void ux_init(void)
{
    ux.init = true;

    ui_style_default();

    ui_clipboard_init();
    ui_tooltip_init();

    ux_topbar_alloc(ux.views + ux_view_topbar);
    ux_status_alloc(ux.views + ux_view_status);

    ux_map_alloc(ux.views + ux_view_map);
    ux_factory_alloc(ux.views + ux_view_factory);

    ux_tapes_alloc(ux.views + ux_view_tapes);
    ux_stars_alloc(ux.views + ux_view_stars);
    ux_mods_alloc(ux.views + ux_view_mods);
    ux_log_alloc(ux.views + ux_view_log);

    ux_star_alloc(ux.views + ux_view_star);
    ux_item_alloc(ux.views + ux_view_item);
    ux_pills_alloc(ux.views + ux_view_pills);
    ux_energy_alloc(ux.views + ux_view_energy);
    ux_workers_alloc(ux.views + ux_view_workers);

    ux_man_alloc(ux.views + ux_view_man);

    ux_reset();
}

void ux_free()
{
    for (enum ux_view view = 0; view < ux_view_len; ++view) {
        struct ux_view_state *state = ux.views + view;
        if (state->fn.free) state->fn.free(state->state);
    }

    ui_clipboard_free();
    ui_tooltip_free();
}

void *ux_state(enum ux_view view)
{
    assert(view > 0 && view < ux_view_len);
    return ux.views[view].state;
}

enum ux_view ux_slot(enum ux_slot slot)
{
    size_t ix = u64_ctz(slot);
    assert(u64_pop(slot) == 1 && ix < ux_slot_len);
    return ux.slots[ix];
}


// -----------------------------------------------------------------------------
// callbacks
// -----------------------------------------------------------------------------

void ux_update_state(void)
{
    // We don't want to run this in tests where we have no ux.
    if (!ux.init) return;

    void update_view(enum ux_view view)
    {
        struct ux_view_state *state = ux.views + view;
        if (state->fn.update_state)
            state->fn.update_state(state->state);
    }

    update_view(ux_view_topbar);
    update_view(ux_view_status);
    for (size_t i = 0; i < ux_slot_len; ++i)
        update_view(ux.slots[i]);
}

void ux_update_frame(void)
{
    void update_view(enum ux_view view)
    {
        struct ux_view_state *state = ux.views + view;
        if (state->fn.update_frame)
            state->fn.update_frame(state->state);
    }

    update_view(ux_view_topbar);
    update_view(ux_view_status);
    for (size_t i = 0; i < ux_slot_len; ++i)
        update_view(ux.slots[i]);
}

static void ux_event_shortcuts(void)
{
    for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev)) {
        if (ev->state != ev_state_down) continue;
        if (ev->mods != ev_mods_alt) continue;

        switch (ev->c)
        {

        case 's': { proxy_save(); ev_consume_key(ev); break; }
        case 'o': { proxy_load(); break; }

        case '1': { proxy_set_speed(speed_slow); ev_consume_key(ev); break; }
        case '2': { proxy_set_speed(speed_fast); ev_consume_key(ev); break; }
        case '3': { proxy_set_speed(speed_faster); ev_consume_key(ev); break; }
        case '4': { proxy_set_speed(speed_fastest); ev_consume_key(ev); break; }
        case ev_key_space: {
            if (proxy_speed() == speed_pause)
                proxy_set_speed(speed_slow);
            else proxy_set_speed(speed_pause);

            ev_consume_key(ev);
            return;
        }

        case 't': { ux_toggle(ux_view_tapes); ev_consume_key(ev); break; }
        case 'a': { ux_toggle(ux_view_stars); ev_consume_key(ev); break; }
        case 'm': { ux_toggle(ux_view_mods); ev_consume_key(ev); break; }
        case 'l': { ux_toggle(ux_view_log); ev_consume_key(ev); break; }
        case 'h': { ux_toggle(ux_view_man); ev_consume_key(ev); break; }
        case 'q': { engine_quit(); ev_consume_key(ev); break; }

        default: { break; }
        }
    }

}

void ux_event(void)
{
    void event_view(enum ux_view view)
    {
        struct ux_view_state *state = ux.views + view;

        if (state->panel) {
            switch (ui_panel_event(state->panel))
            {
            case ui_panel_ev_close: { ux_hide(view); } // fallthrough
            case ui_panel_ev_skip: { return; }
            default: { break; }
            }
        }

        if (state->fn.event) state->fn.event(state->state);
        if (state->panel) ui_panel_event_consume(state->panel);
    }

    void event_slot(enum ux_slot slot)
    {
        event_view(ux.slots[u64_ctz(slot)]);
    }

    ux_event_shortcuts();
    event_view(ux_view_topbar);
    event_view(ux_view_status);
    event_slot(ux_slot_left);
    event_slot(ux_slot_right);
    event_slot(ux_slot_right_sub);
    event_slot(ux_slot_back);
}

void ux_render(void)
{
    struct rect area = engine_area();
    struct ui_layout layout = ui_layout_new(rect_pos(area), rect_dim(area));

    void render_view(enum ux_view view)
    {
        struct ui_layout inner = layout;
        struct ux_view_state *state = ux.views + view;
        if (state->panel) inner = ui_panel_render(state->panel, &layout);
        if (state->fn.render) state->fn.render(state->state, &inner);
        if (state->panel) render_layer_pop();
    }

    void render_slot(enum ux_slot slot)
    {
        render_view(ux.slots[u64_ctz(slot)]);
    }

    render_slot(ux_slot_back);

    {
        render_layer_push_max();

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_up_down);
        render_view(ux_view_topbar);
        ui_layout_next_row(&layout);

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_down_up);
        render_view(ux_view_status);
        ui_layout_next_row(&layout);

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_up_down);
        render_slot(ux_slot_left);

        ui_layout_dir(&layout, ui_layout_right_left | ui_layout_up_down);
        render_slot(ux_slot_right);
        render_slot(ux_slot_right_sub);

        render_layer_pop();
    }

    {
        render_layer_push_max();

        ui_tooltip_render();

        render_layer_pop();
    }
}


// -----------------------------------------------------------------------------
// visibility
// -----------------------------------------------------------------------------

void ux_reset(void)
{
    for (size_t i = 0; i < ux_slot_len; ++i) {
        enum ux_view *slot = ux.slots + i;
        if (*slot) ux_hide(*slot);
    }

    ux_show(ux_view_map);
    ux_show(ux_view_man);
}

static void ux_update_sub(void)
{
    enum ux_view *sub = ux.slots + u64_ctz(ux_slot_right_sub);
    struct ux_view_state *state = ux.views + *sub;
    if (!state->parent) return;

    enum ux_view *right = ux.slots + u64_ctz(ux_slot_right);
    if (*right == state->parent) return;

    if (state->fn.hide) state->fn.hide(state->state);
    if (state->panel) ui_panel_hide(state->panel);
    *sub = ux_view_nil;
}

void ux_show(enum ux_view view)
{
    assert(view > 0 && view < ux_view_len);

    struct ux_view_state *state = ux.views + view;
    assert(state->view == view);

    for (enum ux_slot slots = state->slots; slots; slots &= slots - 1)
        if (ux.slots[u64_ctz(slots)] == view)
            return;

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);

    for (enum ux_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ux_view *slot = ux.slots + u64_ctz(slots);
        if (!*slot) { *slot = view; return; }
    }

    enum ux_view *slot = ux.slots + u64_ctz(state->slots);
    struct ux_view_state *old = ux.views + *slot;

    if (old->panel) ui_panel_hide(old->panel);
    *slot = view;
    ux_update_sub();
}

void ux_show_slot(enum ux_view view, enum ux_slot slot)
{
    assert(view > 0 && view < ux_view_len);

    if (!slot) ux_show(view);
    assert(u64_pop(slot) == 1);

    struct ux_view_state *state = ux.views + view;
    assert(state->view == view);
    assert((state->slots & slot) == slot);

    for (enum ux_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ux_view *entry = ux.slots + u64_ctz(slots);
        if (*entry != view) continue;
        if (u64_ctz(slots) == u64_ctz(slot)) return;
        *entry = ux_view_nil;
    }

    enum ux_view *entry = ux.slots  + u64_ctz(slot);
    if (*entry) ux_hide(*entry);

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);
    *entry = view;
}

void ux_hide(enum ux_view view)
{
    assert(view > 0 && view < ux_view_len);

    struct ux_view_state *state = ux.views + view;
    assert(state->view == view);

    for (enum ux_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ux_view *slot = ux.slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ux_view_nil;
        ux_update_sub();
        return;
    }

    assert(false);
}

void ux_toggle(enum ux_view view)
{
    assert(view > 0 && view < ux_view_len);

    struct ux_view_state *state = ux.views + view;
    assert(state->view == view);

    for (enum ux_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ux_view *slot = ux.slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ux_view_nil;
        ux_update_sub();
        return;
    }

    ux_show(view);
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void ux_log_msg(enum status_type type, const char *msg, size_t len)
{
    if (!ux.init) return;

    const char *prefix = NULL;
    switch (type) {
    case st_info: { prefix = "inf"; break; }
    case st_warn: { prefix = "wrn"; break; }
    case st_error: { prefix = "err"; break; }
    default: { assert(false); }
    }

    ux_status_set(type, msg, len);
    fprintf(stderr, "<%s> %s\n", prefix, msg);
}

void ux_logv(enum status_type type, const char *fmt, va_list args)
{
    static char msg[256] = {0};
    ssize_t len = vsnprintf(msg, sizeof(msg), fmt, args);
    assert(len >= 0);
    ux_log_msg(type, msg, len);
}

void ux_log(enum status_type type, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    ux_logv(type, fmt, args);
    va_end(args);
}
