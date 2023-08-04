/* topbar.c
   Rémi Attab (remi.attab@gmail.com), 13 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "utils/str.h"

static void ui_topbar_free(void *);
static void ui_topbar_update(void *, struct proxy *);
static bool ui_topbar_event(void *, SDL_Event *);
static void ui_topbar_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar
{
    struct ui_panel *panel;
    struct ui_button save, load;
    struct ui_button pause, slow, fast, faster, fastest;
    struct ui_button home, stars, tapes, mods, log;
    struct ui_label coord;
    struct ui_button man;
    struct ui_button close;
};

enum : uint64_t
{
    topbar_ticks_len = 8,
    topbar_coord_len = topbar_ticks_len+1 + coord_str_len+1 + scale_str_len+1,
};

void ui_topbar_alloc(struct ui_view_state *state)
{
    struct ui_topbar *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_topbar) {
        .panel = ui_panel_menu(
                make_dim(ui_layout_inf, ui_st.button.base.height)),

        .save = ui_button_new(ui_str_c("save")),
        .load = ui_button_new(ui_str_c("load")),

        .pause = ui_button_new(ui_str_c("||")),
        .slow = ui_button_new(ui_str_c(">")),
        .fast = ui_button_new(ui_str_c(">>")),
        .faster = ui_button_new(ui_str_c(">>>")),
        .fastest = ui_button_new(ui_str_c(">>|")),

        .home = ui_button_new(ui_str_c("home")),
        .stars = ui_button_new(ui_str_c("stars")),
        .tapes = ui_button_new(ui_str_c("tapes")),
        .mods = ui_button_new(ui_str_c("mods")),
        .log = ui_button_new(ui_str_c("log")),

        .coord = ui_label_new(ui_str_v(topbar_coord_len)),

        .man = ui_button_new(ui_str_c("?")),
        .close = ui_button_new(ui_str_c("x")),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_topbar,
        .panel = ui->panel,
        .fn = {
            .free = ui_topbar_free,
            .update_frame = ui_topbar_update,
            .event = ui_topbar_event,
            .render = ui_topbar_render,
        },
    };
}

static void ui_topbar_free(void *state) {
    struct ui_topbar *ui = state;

    ui_panel_free(ui->panel);

    ui_button_free(&ui->save);
    ui_button_free(&ui->load);

    ui_button_free(&ui->pause);
    ui_button_free(&ui->slow);
    ui_button_free(&ui->fast);
    ui_button_free(&ui->faster);
    ui_button_free(&ui->fastest);

    ui_button_free(&ui->home);
    ui_button_free(&ui->stars);
    ui_button_free(&ui->tapes);
    ui_button_free(&ui->mods);
    ui_button_free(&ui->log);

    ui_label_free(&ui->coord);

    ui_button_free(&ui->man);
    ui_button_free(&ui->close);

    free(ui);
}

static void ui_topbar_update(void *state, struct proxy *proxy)
{
    struct ui_topbar *ui = state;

    ui->pause.disabled = false;
    ui->slow.disabled = false;
    ui->fast.disabled = false;
    ui->faster.disabled = false;
    ui->fastest.disabled = false;

    switch (proxy_speed(proxy))
    {
    case speed_pause: { ui->pause.disabled = true; break; }
    case speed_slow: { ui->slow.disabled = true; break; }
    case speed_fast: { ui->fast.disabled = true; break; }
    case speed_faster: { ui->faster.disabled = true; break; }
    case speed_fastest: { ui->fastest.disabled = true; break; }
    default: { assert(false); }
    }
}

static bool ui_topbar_event(void *state, SDL_Event *ev)
{
    struct ui_topbar *ui = state;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_button_event(&ui->save, ev))) {
        if (ret != ui_action) return true;
        proxy_save(render.proxy);
        return true;
    }

    if ((ret = ui_button_event(&ui->load, ev))) {
        if (ret != ui_action) return true;
        proxy_load(render.proxy);
        return true;
    }

    if ((ret = ui_button_event(&ui->pause, ev))) {
        if (ret != ui_action) return true;
        proxy_set_speed(render.proxy, speed_pause);
        return true;
    }

    if ((ret = ui_button_event(&ui->slow, ev))) {
        if (ret != ui_action) return true;
        proxy_set_speed(render.proxy, speed_slow);
        return true;
    }

    if ((ret = ui_button_event(&ui->fast, ev))) {
        if (ret != ui_action) return true;
        proxy_set_speed(render.proxy, speed_fast);
        return true;
    }

    if ((ret = ui_button_event(&ui->faster, ev))) {
        if (ret != ui_action) return true;
        proxy_set_speed(render.proxy, speed_faster);
        return true;
    }

    if ((ret = ui_button_event(&ui->fastest, ev))) {
        if (ret != ui_action) return true;
        proxy_set_speed(render.proxy, speed_fastest);
        return true;
    }

    if ((ret = ui_button_event(&ui->home, ev))) {
        if (ret != ui_action) return true;
        ui_map_show(proxy_home(render.proxy));
        return true;
    }

    if ((ret = ui_button_event(&ui->stars, ev))) {
        if (ret != ui_action) return true;
        ui_toggle(ui_view_stars);
        return true;
    }

    if ((ret = ui_button_event(&ui->tapes, ev))) {
        if (ret != ui_action) return true;
        ui_toggle(ui_view_tapes);
        return true;
    }

    if ((ret = ui_button_event(&ui->mods, ev))) {
        if (ret != ui_action) return true;
        ui_toggle(ui_view_mods);
        return true;
    }

    if ((ret = ui_button_event(&ui->log, ev))) {
        if (ret != ui_action) return true;
        ui_toggle(ui_view_log);
        return true;
    }

    if ((ret = ui_button_event(&ui->man, ev))) {
        if (ret != ui_action) return true;
        ui_toggle(ui_view_man);
        return true;
    }

    if ((ret = ui_button_event(&ui->close, ev))) {
        if (ret != ui_action) return true;
        sdl_err(SDL_PushEvent(&(SDL_Event) { .type = SDL_QUIT }));
        return true;
    }

    return false;
}

static void topbar_render_coord(
        struct ui_topbar *ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    char buffer[topbar_coord_len] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += str_utoa(proxy_time(render.proxy), it, topbar_ticks_len);
    *it = ' '; it++; assert(it < end);

    coord_scale scale = 0;
    struct coord coord = {0};
    switch (ui_slot(ui_slot_back)) {
    case ui_view_map: {
        scale = ui_map_scale();
        coord = ui_map_coord();
        break;
    }
    case ui_view_factory: {
        scale = ui_factory_scale();
        coord = ui_factory_coord();
        break;
    }
    default: { assert(false); }
    }

    it += coord_str(coord, it, end - it);
    *it = ' '; it++; assert(it < end);

    it += scale_str(scale, it, end - it);
    assert(it <= end);

    ui_str_setv(&ui->coord.str, buffer, sizeof(buffer));
    ui_label_render(&ui->coord, layout, renderer);
}

static void ui_topbar_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_topbar *ui = state;

    ui_button_render(&ui->save, layout, renderer);
    ui_button_render(&ui->load, layout, renderer);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ui->pause, layout, renderer);
    ui_button_render(&ui->slow, layout, renderer);
    ui_button_render(&ui->fast, layout, renderer);
    ui_button_render(&ui->faster, layout, renderer);
    ui_button_render(&ui->fastest, layout, renderer);
    ui_layout_sep_x(layout, 10);

    ui_button_render(&ui->home, layout, renderer);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ui->stars, layout, renderer);
    ui_button_render(&ui->tapes, layout, renderer);
    ui_button_render(&ui->mods, layout, renderer);
    ui_button_render(&ui->log, layout, renderer);

    ui_layout_dir(layout, ui_layout_right_left);
    ui_button_render(&ui->close, layout, renderer);
    ui_layout_sep_x(layout, 10);
    ui_button_render(&ui->man, layout, renderer);
    ui_layout_dir(layout, ui_layout_left_right);

    ui_layout_mid(layout, ui->coord.w.dim.w);
    topbar_render_coord(ui, layout, renderer);
}
