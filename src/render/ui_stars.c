/* ui_stars.c
   Rémi Attab (remi.attab@gmail.com), 16 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sector.h"
#include "render/ui.h"
#include "ui/ui.h"

static void ui_stars_free(void *);
static void ui_stars_update(void *);
static bool ui_stars_event(void *, SDL_Event *);
static void ui_stars_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

struct ui_stars
{
    struct ui_panel *panel;
    struct ui_tree tree;
};

void ui_stars_alloc(struct ui_view_state *state)
{
    struct ui_stars *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_stars) {
        .panel = ui_panel_title(
                make_dim((symbol_cap + 4) * ui_st.font.dim.w, ui_layout_inf),
                ui_str_v(16)),
        .tree = ui_tree_new(make_dim(ui_layout_inf, ui_layout_inf), symbol_cap),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_stars,
        .slots = ui_slot_left,
        .panel = ui->panel,
        .fn = {
            .free = ui_stars_free,
            .update_frame = ui_stars_update,
            .event = ui_stars_event,
            .render = ui_stars_render,
        },
    };
}

static void ui_stars_free(void *state)
{
    struct ui_stars *ui = state;
    ui_panel_free(ui->panel);
    ui_tree_free(&ui->tree);
    free(ui);
}

static void ui_stars_update(void *state)
{
    struct ui_stars *ui = state;
    ui_tree_reset(&ui->tree);

    ui_node parent = ui_node_nil;
    struct coord sector = coord_nil();
    const struct vec64 *list = proxy_chunks();

    size_t count = 0;
    for (size_t i = 0; i < list->len; ++i) {
        struct coord star = coord_from_u64(list->vals[i]);
        if (!coord_eq(coord_sector(star), sector)) {
            sector = coord_sector(star);
            parent = ui_tree_index(&ui->tree);

            struct symbol name = sector_name(sector, proxy_seed());
            ui_str_set_symbol(
                    ui_tree_add(&ui->tree, ui_node_nil, coord_to_u64(sector)), &name);
        }

        ui_str_set_atom(
                ui_tree_add(&ui->tree, parent, coord_to_u64(star)),
                proxy_star_name(star));
        count++;
    }

    ui_str_setf(&ui->panel->title.str, "stars (%zu)", count);
}

static void ui_stars_event_user(struct ui_stars *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_star_select: {
        struct coord star = coord_from_u64((uintptr_t) ev->user.data1);
        ui_tree_select(&ui->tree, coord_to_u64(star));
        return;
    }

    default: { return; }
    }
}

static bool ui_stars_event(void *state, SDL_Event *ev)
{
    struct ui_stars *ui = state;

    if (ev->type == render.event)
        ui_stars_event_user(ui, ev);

    enum ui_ret ret = ui_nil;
    if ((ret = ui_tree_event(&ui->tree, ev))) {
        if (ret != ui_action) return true;

        uint64_t user = ui->tree.selected;
        struct coord coord = coord_from_u64(user);
        if (user && !coord_eq(coord, coord_sector(coord))) {
            ui_star_show(coord);
            ui_map_goto(coord);
        }
        return true;
    }

    return false;
}

static void ui_stars_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_stars *ui = state;
    ui_tree_render(&ui->tree, layout, renderer);
}
