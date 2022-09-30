/* topbar.c
   Rémi Attab (remi.attab@gmail.com), 13 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar
{
    struct ui_panel panel;
    struct ui_button save, load;
    struct ui_button pause, slow, fast, faster, fastest;
    struct ui_button home, stars, tapes, mods, log;
    struct ui_label coord;
    struct ui_button man;
    struct ui_button close;
};

enum
{
    topbar_ticks_len = 8,
    topbar_coord_len = topbar_ticks_len+1 + coord_str_len+1 + scale_str_len+1,
};


struct ui_topbar *ui_topbar_new(void)
{
    struct font *font = font_mono6;
    struct ui_topbar *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_topbar) {
        .panel = ui_panel_menu(make_pos(0, 0), make_dim(render.rect.w, font->glyph_h + 8)),

        .save = ui_button_new(font, ui_str_c("save")),
        .load = ui_button_new(font, ui_str_c("load")),

        .pause = ui_button_new(font, ui_str_c("||")),
        .slow = ui_button_new(font, ui_str_c(">")),
        .fast = ui_button_new(font, ui_str_c(">>")),
        .faster = ui_button_new(font, ui_str_c(">>>")),
        .fastest = ui_button_new(font, ui_str_c(">>|")),

        .home = ui_button_new(font, ui_str_c("home")),
        .stars = ui_button_new(font, ui_str_c("stars")),
        .tapes = ui_button_new(font, ui_str_c("tapes")),
        .mods = ui_button_new(font, ui_str_c("mods")),
        .log = ui_button_new(font, ui_str_c("log")),

        .coord = ui_label_new(ui_str_v(topbar_coord_len)),

        .man = ui_button_new(font, ui_str_c("?")),
        .close = ui_button_new(font, ui_str_c("x")),
    };

    return ui;
}

void ui_topbar_free(struct ui_topbar *ui) {
    ui_panel_free(&ui->panel);

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

int16_t ui_topbar_height(void)
{
    return render.ui.topbar->panel.w.dim.h;
}

static void ui_topbar_update_speed(struct ui_topbar *ui)
{
    ui->pause.disabled = false;
    ui->slow.disabled = false;
    ui->fast.disabled = false;
    ui->faster.disabled = false;
    ui->fastest.disabled = false;

    switch (proxy_speed(render.proxy))
    {
    case speed_pause: { ui->pause.disabled = true; break; }
    case speed_slow: { ui->slow.disabled = true; break; }
    case speed_fast: { ui->fast.disabled = true; break; }
    case speed_faster: { ui->faster.disabled = true; break; }
    case speed_fastest: { ui->fastest.disabled = true; break; }
    default: { assert(false); }
    }
}

bool ui_topbar_event_user(struct ui_topbar *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD:
    case EV_STATE_UPDATE: {
        ui_topbar_update_speed(ui);
        return false;
    }

    default: { return false; }
    }
}

bool ui_topbar_event_shortcuts(struct ui_topbar *ui, SDL_Event *ev)
{
    (void) ui;
    if (!(ev->key.keysym.mod & KMOD_CTRL)) return false;

    switch (ev->key.keysym.sym)
    {

    case SDLK_s: { proxy_save(render.proxy); return true; }
    case SDLK_o: { proxy_load(render.proxy); return true; }

    case SDLK_1: { proxy_set_speed(render.proxy, speed_slow); return true; }
    case SDLK_2: { proxy_set_speed(render.proxy, speed_fast); return true; }
    case SDLK_3: { proxy_set_speed(render.proxy, speed_faster); return true; }
    case SDLK_4: { proxy_set_speed(render.proxy, speed_fastest); return true; }
    case SDLK_SPACE: {
        if (proxy_speed(render.proxy) == speed_pause)
            proxy_set_speed(render.proxy, speed_slow);
        else proxy_set_speed(render.proxy, speed_pause);
        return true;
    }

    case SDLK_m: { render_push_event(EV_MODS_TOGGLE, 0, 0); return true; }
    case SDLK_a: { render_push_event(EV_STARS_TOGGLE, 0, 0); return true; }
    case SDLK_t: { render_push_event(EV_TAPES_TOGGLE, 0, 0); return true; }
    case SDLK_l: { render_push_event(EV_LOG_TOGGLE, 0, 0); return true; }
    case SDLK_h: { render_push_event(EV_MAN_TOGGLE, 0, 0); return true; }
    case SDLK_q: { render_push_quit(); return true; }

    default: { return false; }
    }
}

bool ui_topbar_event(struct ui_topbar *ui, SDL_Event *ev)
{
    if (ev->type == render.event) return ui_topbar_event_user(ui, ev);
    if (ev->type == SDL_KEYDOWN) return ui_topbar_event_shortcuts(ui, ev);

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if ((ret = ui_button_event(&ui->save, ev))) {
        proxy_save(render.proxy);
        return true;
    }

    if ((ret = ui_button_event(&ui->load, ev))) {
        proxy_load(render.proxy);
        return true;
    }

    if ((ret = ui_button_event(&ui->pause, ev))) {
        proxy_set_speed(render.proxy, speed_pause);
        return true;
    }

    if ((ret = ui_button_event(&ui->slow, ev))) {
        proxy_set_speed(render.proxy, speed_slow);
        return true;
    }

    if ((ret = ui_button_event(&ui->fast, ev))) {
        proxy_set_speed(render.proxy, speed_fast);
        return true;
    }

    if ((ret = ui_button_event(&ui->faster, ev))) {
        proxy_set_speed(render.proxy, speed_faster);
        return true;
    }

    if ((ret = ui_button_event(&ui->fastest, ev))) {
        proxy_set_speed(render.proxy, speed_fastest);
        return true;
    }

    if ((ret = ui_button_event(&ui->home, ev))) {
        render_push_event(EV_MAP_GOTO, coord_to_u64(proxy_home(render.proxy)), 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->stars, ev))) {
        render_push_event(EV_STARS_TOGGLE, 0, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->tapes, ev))) {
        render_push_event(EV_TAPES_TOGGLE, 0, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->mods, ev))) {
        render_push_event(EV_MODS_TOGGLE, 0, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->log, ev))) {
        render_push_event(EV_LOG_TOGGLE, 0, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->man, ev))) {
        render_push_event(EV_MAN_TOGGLE, 0, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->close, ev))) {
        sdl_err(SDL_PushEvent(&(SDL_Event) { .type = SDL_QUIT }));
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
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
    if (map_active(render.ui.map)) {
        scale = map_scale(render.ui.map);
        coord = map_coord(render.ui.map);
    }
    else if (factory_active(render.ui.factory)) {
        scale = factory_scale(render.ui.factory);
        coord = factory_coord(render.ui.factory);
    }

    it += coord_str(coord, it, end - it);
    *it = ' '; it++; assert(it < end);

    it += scale_str(scale, it, end - it);
    assert(it <= end);

    ui_str_setv(&ui->coord.str, buffer, sizeof(buffer));
    ui_label_render(&ui->coord, layout, renderer);
}

void ui_topbar_render(struct ui_topbar *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);

    ui_button_render(&ui->save, &layout, renderer);
    ui_button_render(&ui->load, &layout, renderer);
    ui_layout_sep_x(&layout, 10);

    ui_button_render(&ui->pause, &layout, renderer);
    ui_button_render(&ui->slow, &layout, renderer);
    ui_button_render(&ui->fast, &layout, renderer);
    ui_button_render(&ui->faster, &layout, renderer);
    ui_button_render(&ui->fastest, &layout, renderer);
    ui_layout_sep_x(&layout, 10);

    ui_button_render(&ui->home, &layout, renderer);
    ui_layout_sep_x(&layout, 10);
    ui_button_render(&ui->stars, &layout, renderer);
    ui_button_render(&ui->tapes, &layout, renderer);
    ui_button_render(&ui->mods, &layout, renderer);
    ui_button_render(&ui->log, &layout, renderer);

    ui_layout_dir(&layout, ui_layout_left);
    ui_button_render(&ui->close, &layout, renderer);
    ui_layout_sep_x(&layout, 10);
    ui_button_render(&ui->man, &layout, renderer);

    ui_layout_mid(&layout, ui->coord.w.dim.w);
    topbar_render_coord(ui, &layout, renderer);
}
