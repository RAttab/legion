/* ui.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/ui.h"

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

struct ui
{
    enum ui_view slots[ui_slot_len];
    struct ui_view_state views[ui_view_len];
};

struct ui *ui_alloc(void)
{
    struct ui *ui = calloc(1, sizeof(*ui));

    ui_topbar_alloc(ui->views + ui_view_topbar);
    ui_status_alloc(ui->views + ui_view_status);

    ui_map_alloc(ui->views + ui_view_map);
    ui_factory_alloc(ui->views + ui_view_factory);

    ui_tapes_alloc(ui->views + ui_view_tapes);
    ui_stars_alloc(ui->views + ui_view_stars);
    ui_mods_alloc(ui->views + ui_view_mods);
    ui_log_alloc(ui->views + ui_view_log);

    ui_star_alloc(ui->views + ui_view_star);
    ui_item_alloc(ui->views + ui_view_item);
    ui_pills_alloc(ui->views + ui_view_pills);
    ui_energy_alloc(ui->views + ui_view_energy);
    ui_workers_alloc(ui->views + ui_view_workers);

    ui_man_alloc(ui->views + ui_view_man);

    ui_reset(ui);
    return ui;
}

void ui_free(struct ui *ui)
{
    for (enum ui_view view = 0; view < ui_view_len; ++view) {
        struct ui_view_state *state = ui->views + view;
        if (state->fn.free) state->fn.free(state->state);
    }
    free(ui);
}

void *ui_state(struct ui *ui, enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);
    return ui->views[view].state;
}

enum ui_view ui_slot(struct ui *ui, enum ui_slot slot)
{
    size_t ix = u64_ctz(slot);
    assert(u64_pop(slot) == 1 && ix < ui_slot_len);
    return ui->slots[ix];
}


void ui_update_state(struct ui *ui, struct proxy *proxy)
{
    void update_view(enum ui_view view)
    {
        struct ui_view_state *state = ui->views + view;
        if (state->fn.update_state)
            state->fn.update_state(state->state, proxy);
    }

    update_view(ui_view_topbar);
    update_view(ui_view_status);
    for (size_t i = 0; i < ui_slot_len; ++i)
        update_view(ui->slots[i]);
}

void ui_update_frame(struct ui *ui, struct proxy *proxy)
{
    void update_view(enum ui_view view)
    {
        struct ui_view_state *state = ui->views + view;
        if (state->fn.update_frame)
            state->fn.update_frame(state->state, proxy);
    }

    update_view(ui_view_topbar);
    update_view(ui_view_status);
    for (size_t i = 0; i < ui_slot_len; ++i)
        update_view(ui->slots[i]);
}

static bool ui_event_shortcuts(struct ui *ui, SDL_Event *ev)
{
    if (ev->type != SDL_KEYDOWN) return false;
    if (!(ev->key.keysym.mod & KMOD_CTRL)) return false;
    struct proxy *proxy = render.proxy;

    switch (ev->key.keysym.sym)
    {

    case SDLK_s: { proxy_save(proxy); return true; }
    case SDLK_o: { proxy_load(proxy); return true; }

    case SDLK_1: { proxy_set_speed(proxy, speed_slow); return true; }
    case SDLK_2: { proxy_set_speed(proxy, speed_fast); return true; }
    case SDLK_3: { proxy_set_speed(proxy, speed_faster); return true; }
    case SDLK_4: { proxy_set_speed(proxy, speed_fastest); return true; }
    case SDLK_SPACE: {
        if (proxy_speed(proxy) == speed_pause)
            proxy_set_speed(proxy, speed_slow);
        else proxy_set_speed(proxy, speed_pause);
        return true;
    }

    case SDLK_t: { ui_toggle(ui, ui_view_tapes); return true; }
    case SDLK_a: { ui_toggle(ui, ui_view_stars); return true; }
    case SDLK_m: { ui_toggle(ui, ui_view_mods); return true; }
    case SDLK_l: { ui_toggle(ui, ui_view_log); return true; }
    case SDLK_h: { ui_toggle(ui, ui_view_man); return true; }
    case SDLK_q: { render_quit(); return true; }

    default: { return false; }
    }
}

void ui_event(struct ui *ui, SDL_Event *ev)
{
    bool event_view(enum ui_view view)
    {
        struct ui_view_state *state = ui->views + view;

        if (state->panel) {
            enum ui_ret ret = ui_panel_event(state->panel, ev);
            if (ret == ui_action && !ui_panel_is_visible(state->panel))
                ui_hide(ui, view);
            if (ret) return ret != ui_skip;
        }

        if (state->fn.event && state->fn.event(state->state, ev))
            return true;

        if (state->panel && ui_panel_event_consume(state->panel, ev))
            return true;

        return false;
    }

    bool event_slot(enum ui_slot slot)
    {
        return event_view(ui->slots[u64_ctz(slot)]);
    }

    if (ui_event_shortcuts(ui, ev)) return;
    if (event_view(ui_view_topbar)) return;
    if (event_view(ui_view_status)) return;
    if (event_slot(ui_slot_left)) return;
    if (event_slot(ui_slot_right)) return;
    if (event_slot(ui_slot_right_sub)) return;
    if (event_slot(ui_slot_back)) return;
}

void ui_render(struct ui *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_layout_new(
            make_pos(0, 0),
            make_dim(render.rect.w, render.rect.h));

    void render_view(enum ui_view view)
    {
        struct ui_layout inner = layout;
        struct ui_view_state *state = ui->views + view;
        if (state->panel) inner = ui_panel_render(state->panel, &layout, renderer);
        if (state->fn.render) state->fn.render(state->state, &inner, renderer);
    }

    void render_slot(enum ui_slot slot)
    {
        render_view(ui->slots[u64_ctz(slot)]);
    }

    render_slot(ui_slot_back);

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
}

void ui_reset(struct ui *ui)
{
    for (size_t i = 0; i < ui_slot_len; ++i) {
        enum ui_view *slot = ui->slots + i;
        if (*slot) ui_hide(ui, *slot);
    }

    ui_show(ui, ui_view_map);
    ui_show(ui, ui_view_man);
}

static void ui_update_sub(struct ui *ui)
{
    enum ui_view *sub = ui->slots + u64_ctz(ui_slot_right_sub);
    struct ui_view_state *state = ui->views + *sub;
    if (!state->parent) return;

    enum ui_view *right = ui->slots + u64_ctz(ui_slot_right);
    if (*right == state->parent) return;

    if (state->fn.hide) state->fn.hide(state->state);
    if (state->panel) ui_panel_hide(state->panel);
    *sub = ui_view_nil;
}

void ui_show(struct ui *ui, enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui->views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1)
        if (ui->slots[u64_ctz(slots)] == view)
            return;

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui->slots + u64_ctz(slots);
        if (!*slot) { *slot = view; return; }
    }

    enum ui_view *slot = ui->slots + u64_ctz(state->slots);
    struct ui_view_state *old = ui->views + *slot;

    if (old->panel) ui_panel_hide(old->panel);
    *slot = view;
    ui_update_sub(ui);
}

void ui_show_slot(struct ui *ui, enum ui_view view, enum ui_slot slot)
{
    assert(view > 0 && view < ui_view_len);

    if (!slot) ui_show(ui, view);
    assert(u64_pop(slot) == 1);

    struct ui_view_state *state = ui->views + view;
    assert(state->view == view);
    assert((state->slots & slot) == slot);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *entry = ui->slots + u64_ctz(slots);
        if (*entry != view) continue;
        if (u64_ctz(slots) == u64_ctz(slot)) return;
        *entry = ui_view_nil;
    }

    enum ui_view *entry = ui->slots  + u64_ctz(slot);
    if (*entry) ui_hide(ui, *entry);

    if (state->panel) ui_panel_show(state->panel);
    if (state->fn.show) state->fn.show(state->state);
    *entry = view;
}

void ui_hide(struct ui *ui, enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui->views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui->slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ui_view_nil;
        ui_update_sub(ui);
        return;
    }

    assert(false);
}

void ui_toggle(struct ui *ui, enum ui_view view)
{
    assert(view > 0 && view < ui_view_len);

    struct ui_view_state *state = ui->views + view;
    assert(state->view == view);

    for (enum ui_slot slots = state->slots; slots; slots &= slots - 1) {
        enum ui_view *slot = ui->slots + u64_ctz(slots);
        if (*slot != view) continue;

        if (state->fn.hide) state->fn.hide(state->state);
        if (state->panel) ui_panel_hide(state->panel);
        *slot = ui_view_nil;
        ui_update_sub(ui);
        return;
    }

    ui_show(ui, view);
}
