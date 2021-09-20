/* ui_stars.c
   Rémi Attab (remi.attab@gmail.com), 16 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"


// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

struct ui_stars
{
    struct ui_panel panel;
    struct ui_tree tree;
};

static struct font *ui_stars_font(void) { return font_mono6; }

struct ui_stars *ui_stars_new(void)
{
    struct font *font = ui_stars_font();
    struct pos pos = make_pos(0, ui_topbar_height());
    struct dim dim = make_dim(
            (symbol_cap + 4) * font->glyph_w,
            core.rect.h - pos.y - ui_status_height());

    struct ui_stars *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_stars) {
        .panel = ui_panel_title(pos, dim, ui_str_c("stars")),
        .tree = ui_tree_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                font, ui_str_v(symbol_cap)),
    };

    ui_panel_hide(&ui->panel);
    return ui;
}


void ui_stars_free(struct ui_stars *ui) {
    ui_panel_free(&ui->panel);
    ui_tree_free(&ui->tree);
    free(ui);
}

static void ui_stars_update(struct ui_stars *ui)
{
    struct world *world = core.state.world;
    ui_tree_reset(&ui->tree);

    ui_node_t parent = ui_node_nil;
    struct coord sector = coord_nil();
    struct world_chunk_it it = world_chunk_it(world);

    for (struct coord ret = world_chunk_next(world, &it);
         !coord_is_nil(ret); ret = world_chunk_next(world, &it))
    {
        if (!coord_eq(coord_sector(ret), sector)) {
            sector = coord_sector(ret);
            parent = ui_tree_index(&ui->tree);
            ui_str_setf(ui_tree_add(&ui->tree, ui_node_nil, coord_to_u64(sector)),
                    "%02x.%02x x %02x.%02x",
                    (sector.x >> coord_sector_bits) & 0xFF,
                    (sector.x >> (coord_sector_bits + coord_area_bits)),
                    (sector.y >> coord_sector_bits) & 0xFF,
                    (sector.y >> (coord_sector_bits + coord_area_bits)));
        }

        ui_str_set_atom(
                ui_tree_add(&ui->tree, parent, coord_to_u64(ret)),
                world_star_name(world, ret));
    }
}

static bool ui_stars_event_user(struct ui_stars *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_stars_update(ui);
        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel))
            ui_stars_update(ui);
        return false;
    }

    case EV_STARS_TOGGLE: {
        if (ui_panel_is_visible(&ui->panel))
            ui_panel_hide(&ui->panel);
        else {
            ui_stars_update(ui);
            ui_panel_show(&ui->panel);
        }
        return false;
    }

    case EV_STAR_SELECT: {
        struct coord coord = coord_from_u64((uintptr_t) ev->user.data1);
        ui_tree_select(&ui->tree, coord_to_u64(coord));
        return false;
    }

    case EV_STAR_CLEAR: {
        ui_tree_clear(&ui->tree);
        return false;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_MODS_TOGGLE:
    case EV_MOD_SELECT:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
}

bool ui_stars_event(struct ui_stars *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_stars_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if ((ret = ui_tree_event(&ui->tree, ev))) {
        if (ret != ui_action) return true;

        uint64_t user = ui->tree.selected;
        struct coord coord = coord_from_u64(user);
        if (user && !coord_eq(coord, coord_sector(coord))) {
            core_push_event(EV_STAR_SELECT, user, 0);
            core_push_event(EV_MAP_GOTO, user, 0);
        }
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_stars_render(struct ui_stars *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_tree_render(&ui->tree, &layout, renderer);
}
