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
    struct ui_button home, stars, tapes, mods, log;
    struct ui_label coord;
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
        .panel = ui_panel_menu(make_pos(0, 0), make_dim(core.rect.w, font->glyph_h + 8)),
        .save = ui_button_new(font, ui_str_c("save")),
        .load = ui_button_new(font, ui_str_c("load")),
        .home = ui_button_new(font, ui_str_c("home")),
        .stars = ui_button_new(font, ui_str_c("stars")),
        .tapes = ui_button_new(font, ui_str_c("tapes")),
        .mods = ui_button_new(font, ui_str_c("mods")),
        .log = ui_button_new(font, ui_str_c("log")),
        .coord = ui_label_new(font, ui_str_v(topbar_coord_len)),
        .close = ui_button_new(font, ui_str_c("x")),
    };

    return ui;
}

void ui_topbar_free(struct ui_topbar *ui) {
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->save);
    ui_button_free(&ui->load);
    ui_button_free(&ui->home);
    ui_button_free(&ui->stars);
    ui_button_free(&ui->tapes);
    ui_button_free(&ui->mods);
    ui_button_free(&ui->log);
    ui_label_free(&ui->coord);
    ui_button_free(&ui->close);
    free(ui);
}

int16_t ui_topbar_height(void)
{
    return core.ui.topbar->panel.w.dim.h;
}

bool ui_topbar_event(struct ui_topbar *ui, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;

    if ((ret = ui_button_event(&ui->save, ev))) {
        core_save();
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->load, ev))) {
        core_load();
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->home, ev))) {
        core_push_event(EV_MAP_GOTO, coord_to_u64(core.state.home), 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->stars, ev))) {
        core_push_event(EV_STARS_TOGGLE, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->tapes, ev))) {
        core_push_event(EV_TAPES_TOGGLE, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->mods, ev))) {
        core_push_event(EV_MODS_TOGGLE, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->log, ev))) {
        core_push_event(EV_LOG_TOGGLE, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->close, ev))) {
        sdl_err(SDL_PushEvent(&(SDL_Event) { .type = SDL_QUIT }));
        return ret == ui_consume;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

static void topbar_render_coord(
        struct ui_topbar *ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    char buffer[topbar_coord_len] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += str_utoa(world_time(core.state.world), it, topbar_ticks_len);
    *it = ' '; it++; assert(it < end);

    scale_t scale = 0;
    struct coord coord = {0};
    if (map_active(core.ui.map)) {
        scale = map_scale(core.ui.map);
        coord = map_coord(core.ui.map);
    }
    else if (factory_active(core.ui.factory)) {
        scale = factory_scale(core.ui.factory);
        coord = factory_coord(core.ui.factory);
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
    ui->load.disabled = core.state.loading;
    ui_button_render(&ui->load, &layout, renderer);
    ui_layout_sep_x(&layout, 10);

    ui_button_render(&ui->home, &layout, renderer);
    ui_layout_sep_x(&layout, 10);
    ui_button_render(&ui->stars, &layout, renderer);
    ui_button_render(&ui->tapes, &layout, renderer);
    ui_button_render(&ui->mods, &layout, renderer);
    ui_button_render(&ui->log, &layout, renderer);

    ui_layout_mid(&layout, &ui->coord.w);
    topbar_render_coord(ui, &layout, renderer);

    ui_layout_right(&layout, &ui->close.w);
    ui_button_render(&ui->close, &layout, renderer);
}
