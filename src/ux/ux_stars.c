/* ux_stars.c
   Rémi Attab (remi.attab@gmail.com), 16 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_stars_free(void *);
static void ux_stars_update(void *);
static void ux_stars_event(void *);
static void ux_stars_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

struct ux_stars
{
    struct ui_panel *panel;
    struct ui_tree tree;
};

void ux_stars_alloc(struct ux_view_state *state)
{
    struct ux_stars *ux = mem_alloc_t(ux);

    struct dim cell = engine_cell();
    *ux = (struct ux_stars) {
        .panel = ui_panel_title(
                make_dim((symbol_cap + 4) * cell.w, ui_layout_inf),
                ui_str_v(16)),
        .tree = ui_tree_new(make_dim(ui_layout_inf, ui_layout_inf), symbol_cap),
    };

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_stars,
        .slots = ux_slot_left,
        .panel = ux->panel,
        .fn = {
            .free = ux_stars_free,
            .update = ux_stars_update,
            .event = ux_stars_event,
            .render = ux_stars_render,
        },
    };
}

static void ux_stars_free(void *state)
{
    struct ux_stars *ux = state;
    ui_panel_free(ux->panel);
    ui_tree_free(&ux->tree);
    mem_free(ux);
}

static void ux_stars_update(void *state)
{
    struct ux_stars *ux = state;
    ui_tree_reset(&ux->tree);

    ui_tree_node parent = ui_tree_node_nil;
    struct coord sector = coord_nil();
    const struct vec64 *list = proxy_chunks();

    size_t count = 0;
    for (size_t i = 0; i < list->len; ++i) {
        struct coord star = coord_from_u64(list->vals[i]);
        if (!coord_eq(coord_sector(star), sector)) {
            sector = coord_sector(star);
            parent = ui_tree_index(&ux->tree);

            struct symbol name = sector_name(sector, proxy_seed());
            ui_str_set_symbol(
                    ui_tree_add(&ux->tree, ui_tree_node_nil, coord_to_u64(sector)), &name);
        }

        ui_str_set_atom(
                ui_tree_add(&ux->tree, parent, coord_to_u64(star)),
                proxy_star_name(star));
        count++;
    }

    ui_str_setf(&ux->panel->title.str, "stars (%zu)", count);
}

static void ux_stars_event(void *state)
{
    struct ux_stars *ux = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        ui_tree_select(&ux->tree, coord_to_u64(ev->star));

    if (ui_tree_event(&ux->tree)) {
        uint64_t user = ux->tree.selected;
        struct coord coord = coord_from_u64(user);
        if (user && !coord_eq(coord, coord_sector(coord))) {
            ux_star_show(coord);
            ux_map_goto(coord);
        }
    }
}

static void ux_stars_render(void *state, struct ui_layout *layout)
{
    struct ux_stars *ux = state;
    ui_tree_render(&ux->tree, layout);
}
