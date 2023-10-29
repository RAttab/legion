/* ui.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "ux/ui.h"

#include <stdarg.h>

// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

const char *ui_slot_str(enum ui_slot slot)
{
    switch (slot)
    {
    case ui_slot_nil:       { return "nil"; }
    case ui_slot_back:      { return "back"; }
    case ui_slot_right:     { return "right"; }
    case ui_slot_right_sub: { return "rsub"; }
    case ui_slot_left:      { return "left"; }
    default:                { return "?"; }
    }
}

const char *ui_view_str(enum ui_view view)
{
    switch (view)
    {
    case ui_view_nil:     { return "nil"; }

    case ui_view_topbar:  { return "topbar"; }
    case ui_view_status:  { return "status"; }

    case ui_view_map:     { return "map"; }
    case ui_view_factory: { return "factory"; }

    case ui_view_tapes:   { return "tapes"; }
    case ui_view_stars:   { return "stars"; }
    case ui_view_mods:    { return "mods"; }
    case ui_view_log:     { return "log"; }

    case ui_view_star:    { return "star"; }
    case ui_view_item:    { return "item"; }
    case ui_view_pills:   { return "pills"; }
    case ui_view_energy:  { return "energy"; }
    case ui_view_workers: { return "workers"; }

    case ui_view_man:     { return "man"; }

    default:              { return "?"; }
    }
}



// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

struct
{
    bool init;
    enum ui_view slots[ui_slot_len];
    struct ui_view_state views[ui_view_len];
} ui = { .init = false };


void ui_init(void)
{
    ui.init = true;

    ui_style_default();

    ui_clipboard_init();
    ui_tooltip_init();

    ui_topbar_alloc(ui.views + ui_view_topbar);
    ui_status_alloc(ui.views + ui_view_status);

    ui_map_alloc(ui.views + ui_view_map);
    ui_factory_alloc(ui.views + ui_view_factory);

    ui_tapes_alloc(ui.views + ui_view_tapes);
    ui_stars_alloc(ui.views + ui_view_stars);
    ui_mods_alloc(ui.views + ui_view_mods);
    ui_log_alloc(ui.views + ui_view_log);

    ui_star_alloc(ui.views + ui_view_star);
    ui_item_alloc(ui.views + ui_view_item);
    ui_pills_alloc(ui.views + ui_view_pills);
    ui_energy_alloc(ui.views + ui_view_energy);
    ui_workers_alloc(ui.views + ui_view_workers);

    ui_man_alloc(ui.views + ui_view_man);

    ui_reset();
}

void ui_free()
{
    for (enum ui_view view = 0; view < ui_view_len; ++view) {
        struct ui_view_state *state = ui.views + view;
        if (state->fn.free) state->fn.free(state->state);
    }

    ui_clipboard_free();
    ui_tooltip_free();
}

void *ui_state(enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);
    return ui.views[view].state;
}

enum ui_view ui_slot(enum ui_slot slot)
{
    size_t ix = u64_ctz(slot);
    assert(u64_pop(slot) == 1 && ix < ui_slot_len);
    return ui.slots[ix];
}


// -----------------------------------------------------------------------------
// callbacks
// -----------------------------------------------------------------------------

void ui_update_state(void)
{
    // We don't want to run this in tests where we have no ui.
    if (!ui.init) return;

    void update_view(enum ui_view view)
    {
        struct ui_view_state *state = ui.views + view;
        if (state->fn.update_state)
            state->fn.update_state(state->state);
    }

    update_view(ui_view_topbar);
    update_view(ui_view_status);
    for (size_t i = 0; i < ui_slot_len; ++i)
        update_view(ui.slots[i]);
}

void ui_update_frame(void)
{
    void update_view(enum ui_view view)
    {
        struct ui_view_state *state = ui.views + view;
        if (state->fn.update_frame)
            state->fn.update_frame(state->state);
    }

    update_view(ui_view_topbar);
    update_view(ui_view_status);
    for (size_t i = 0; i < ui_slot_len; ++i)
        update_view(ui.slots[i]);
}

static void ui_event_shortcuts(void)
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

        case 't': { ui_toggle(ui_view_tapes); ev_consume_key(ev); break; }
        case 'a': { ui_toggle(ui_view_stars); ev_consume_key(ev); break; }
        case 'm': { ui_toggle(ui_view_mods); ev_consume_key(ev); break; }
        case 'l': { ui_toggle(ui_view_log); ev_consume_key(ev); break; }
        case 'h': { ui_toggle(ui_view_man); ev_consume_key(ev); break; }
        case 'q': { engine_quit(); ev_consume_key(ev); break; }

        default: { break; }
        }
    }

}

void ui_event(void)
{
    void event_view(enum ui_view view)
    {
        struct ui_view_state *state = ui.views + view;

        if (state->panel) {
            switch (ui_panel_event(state->panel))
            {
            case ui_panel_ev_close: { ui_hide(view); } // fallthrough
            case ui_panel_ev_skip: { return; }
            default: { break; }
            }
        }

        if (state->fn.event) state->fn.event(state->state);
        if (state->panel) ui_panel_event_consume(state->panel);
    }

    void event_slot(enum ui_slot slot)
    {
        event_view(ui.slots[u64_ctz(slot)]);
    }

    ui_event_shortcuts();
    event_view(ui_view_topbar);
    event_view(ui_view_status);
    event_slot(ui_slot_left);
    event_slot(ui_slot_right);
    event_slot(ui_slot_right_sub);
    event_slot(ui_slot_back);
}

void ui_render(void)
{
    struct rect area = engine_area();
    struct ui_layout layout = ui_layout_new(rect_pos(area), rect_dim(area));

    void render_view(enum ui_view view)
    {
        struct ui_layout inner = layout;
        struct ui_view_state *state = ui.views + view;
        if (state->panel) inner = ui_panel_render(state->panel, &layout);
        if (state->fn.render) state->fn.render(state->state, &inner);
        if (state->panel) render_layer_pop();
    }

    void render_slot(enum ui_slot slot)
    {
        render_view(ui.slots[u64_ctz(slot)]);
    }

    render_slot(ui_slot_back);

    {
        render_layer_push_max();

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_up_down);
        render_view(ui_view_topbar);
        ui_layout_next_row(&layout);

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_down_up);
        render_view(ui_view_status);
        ui_layout_next_row(&layout);

        ui_layout_dir(&layout, ui_layout_left_right | ui_layout_up_down);
        render_slot(ui_slot_left);

        ui_layout_dir(&layout, ui_layout_right_left | ui_layout_up_down);
        render_slot(ui_slot_right);
        render_slot(ui_slot_right_sub);

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

void ui_reset(void)
{
    for (size_t i = 0; i < ui_slot_len; ++i) {
        enum ui_view *slot = ui.slots + i;
        if (*slot) ui_hide(*slot);
    }

    ui_show(ui_view_map);
    ui_show(ui_view_man);
}

static void ui_update_sub(void)
{
    enum ui_view *sub = ui.slots + u64_ctz(ui_slot_right_sub);
    struct ui_view_state *state = ui.views + *sub;
    if (!state->parent) return;

    enum ui_view *right = ui.slots + u64_ctz(ui_slot_right);
    if (*right == state->parent) return;

    if (state->fn.hide) state->fn.hide(state->state);
    if (state->panel) ui_panel_hide(state->panel);
    *sub = ui_view_nil;
}

void ui_show(enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui.views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1)
        if (ui.slots[u64_ctz(slots)] == view)
            return;

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui.slots + u64_ctz(slots);
        if (!*slot) { *slot = view; return; }
    }

    enum ui_view *slot = ui.slots + u64_ctz(state->slots);
    struct ui_view_state *old = ui.views + *slot;

    if (old->panel) ui_panel_hide(old->panel);
    *slot = view;
    ui_update_sub();
}

void ui_show_slot(enum ui_view view, enum ui_slot slot)
{
    assert(view > 0 && view < ui_view_len);

    if (!slot) ui_show(view);
    assert(u64_pop(slot) == 1);

    struct ui_view_state *state = ui.views + view;
    assert(state->view == view);
    assert((state->slots & slot) == slot);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *entry = ui.slots + u64_ctz(slots);
        if (*entry != view) continue;
        if (u64_ctz(slots) == u64_ctz(slot)) return;
        *entry = ui_view_nil;
    }

    enum ui_view *entry = ui.slots  + u64_ctz(slot);
    if (*entry) ui_hide(*entry);

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);
    *entry = view;
}

void ui_hide(enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui.views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui.slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ui_view_nil;
        ui_update_sub();
        return;
    }

    assert(false);
}

void ui_toggle(enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui.views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui.slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ui_view_nil;
        ui_update_sub();
        return;
    }

    ui_show(view);
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void ui_log_msg(enum status_type type, const char *msg, size_t len)
{
    if (!ui.init) return;

    const char *prefix = NULL;
    switch (type) {
    case st_info: { prefix = "inf"; break; }
    case st_warn: { prefix = "wrn"; break; }
    case st_error: { prefix = "err"; break; }
    default: { assert(false); }
    }

    ui_status_set(type, msg, len);
    fprintf(stderr, "<%s> %s\n", prefix, msg);
}

void ui_logv(enum status_type type, const char *fmt, va_list args)
{
    static char msg[256] = {0};
    ssize_t len = vsnprintf(msg, sizeof(msg), fmt, args);
    assert(len >= 0);
    ui_log_msg(type, msg, len);
}

void ui_log(enum status_type type, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    ui_logv(type, fmt, args);
    va_end(args);
}