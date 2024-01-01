/* ux_energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_energy_free(void *);
static void ux_energy_hide(void *);
static void ux_energy_update(void *);
static void ux_energy_event(void *);
static void ux_energy_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

enum : unsigned
{
    ux_energy_stored = 0,
    ux_energy_fusion,
    ux_energy_solar,
    ux_energy_burner,
    ux_energy_kwheel,
    ux_energy_consumed,
    ux_energy_saved,
    ux_energy_need,
};

struct ux_energy
{
    struct coord star;
    world_ts last_t;

    struct ui_panel *panel;
    struct ui_histo histo;
};


void ux_energy_alloc(struct ux_view_state *state)
{
    struct ui_histo_series series[] = {
        [ux_energy_stored] =   { 0, "stored", ui_st.rgba.energy.stored, true },
        [ux_energy_fusion] =   { 0, "fusion", ui_st.rgba.energy.fusion, true },
        [ux_energy_solar] =    { 0, "solar", ui_st.rgba.energy.solar, false },
        [ux_energy_burner] =   { 0, "burner", ui_st.rgba.energy.burner, false },
        [ux_energy_kwheel] =   { 0, "k-wheel", ui_st.rgba.energy.kwheel, false },

        [ux_energy_consumed] = { 1, "consumed", ui_st.rgba.energy.consumed, true },
        [ux_energy_saved] =    { 1, "saved", ui_st.rgba.energy.saved, true },
        [ux_energy_need] =     { 1, "need", ui_st.rgba.energy.need, true },
    };

    struct ux_energy *ux = calloc(1, sizeof(*ux));
    struct dim cell = engine_cell();
    *ux = (struct ux_energy) {
        .star = coord_nil(),

        .panel = ui_panel_title(
                make_dim(77 * cell.w, ui_layout_inf),
                ui_str_c("energy")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_energy,
        .parent = ux_view_star,
        .slots = ux_slot_right_sub,
        .panel = ux->panel,
        .fn = {
            .free = ux_energy_free,
            .hide = ux_energy_hide,
            .update = ux_energy_update,
            .event = ux_energy_event,
            .render = ux_energy_render,
        },
    };
}

static void ux_energy_free(void *state)
{
    struct ux_energy *ux = state;
    ui_panel_free(ux->panel);
    ui_histo_free(&ux->histo);
    free(ux);
}

void ux_energy_show(struct coord star)
{
    struct ux_energy *ux = ux_state(ux_view_energy);

    ux->star = star;
    if (coord_is_nil(ux->star)) { ux_hide(ux_view_energy); return; }

    ux_energy_update(ux);
    ux_show(ux_view_energy);
    proxy_steps(cmd_steps_energy);
}

static void ux_energy_hide(void *)
{
    proxy_steps(cmd_steps_nil);
}

static void ux_energy_update(void *state)
{
    struct ux_energy *ux = state;

    struct chunk *chunk = proxy_chunk(ux->star);
    if (!chunk) { ux_energy_show(coord_nil()); return; }

    const struct tech *tech = proxy_tech();
    ui_histo_series(&ux->histo, ux_energy_solar)->visible = tech_known(tech, item_solar);
    ui_histo_series(&ux->histo, ux_energy_burner)->visible = tech_known(tech, item_burner);
    ui_histo_series(&ux->histo, ux_energy_kwheel)->visible = tech_known(tech, item_kwheel);

    ui_histo_update_legend(&ux->histo);

    world_ts t = 0;
    struct energy energy = {0};
    const struct star *star = chunk_star(chunk);

    while (proxy_steps_next_energy(&t, &energy)) {
        if (ux->last_t == t) continue;
        ux->last_t = t;

        ui_histo_advance(&ux->histo, t);
        ui_histo_push(&ux->histo, ux_energy_consumed, energy.consumed);
        ui_histo_push(&ux->histo, ux_energy_saved, energy.item.battery.stored);
        ui_histo_push(&ux->histo, ux_energy_need, energy.need - energy.consumed);
        ui_histo_push(&ux->histo, ux_energy_stored, energy.item.battery.produced);
        ui_histo_push(&ux->histo, ux_energy_fusion, energy.item.fusion.produced);
        ui_histo_push(&ux->histo, ux_energy_solar, energy_prod_solar(&energy, star));
        ui_histo_push(&ux->histo, ux_energy_burner, energy.item.burner);

        // won't be accurate but it's not even implemented yet so... /shrug
        ui_histo_push(&ux->histo, ux_energy_kwheel, energy_prod_kwheel(&energy, star));
    }
}

static void ux_energy_event(void *state)
{
    struct ux_energy *ux = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ux->star, ev->star))
            ux_energy_show(ev->star);

    ui_histo_event(&ux->histo);
}

static void ux_energy_render(void *state, struct ui_layout *layout)
{
    struct ux_energy *ux = state;
    ui_histo_render(&ux->histo, layout);
}
