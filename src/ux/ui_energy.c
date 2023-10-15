/* ui_energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/chunk.h"
#include "game/energy.h"

static void ui_energy_free(void *);
static void ui_energy_update_state(void *);
static void ui_energy_update_frame(void *);
static void ui_energy_event(void *);
static void ui_energy_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

enum
{
    ui_energy_stored = 0,
    ui_energy_fusion,
    ui_energy_solar,
    ui_energy_burner,
    ui_energy_kwheel,
    ui_energy_consumed,
    ui_energy_saved,
    ui_energy_need,
};

struct ui_energy
{
    struct coord star;
    world_ts last_t;

    struct ui_panel *panel;
    struct ui_histo histo;
};


void ui_energy_alloc(struct ui_view_state *state)
{
    struct ui_histo_series series[] = {
        [ui_energy_stored] =   { 0, "stored", ui_st.rgba.energy.stored, true },
        [ui_energy_fusion] =   { 0, "fusion", ui_st.rgba.energy.fusion, true },
        [ui_energy_solar] =    { 0, "solar", ui_st.rgba.energy.solar, false },
        [ui_energy_burner] =   { 0, "burner", ui_st.rgba.energy.burner, false },
        [ui_energy_kwheel] =   { 0, "k-wheel", ui_st.rgba.energy.kwheel, false },

        [ui_energy_consumed] = { 1, "consumed", ui_st.rgba.energy.consumed, true },
        [ui_energy_saved] =    { 1, "saved", ui_st.rgba.energy.saved, true },
        [ui_energy_need] =     { 1, "need", ui_st.rgba.energy.need, true },
    };

    struct ui_energy *ui = calloc(1, sizeof(*ui));
    struct dim cell = engine_cell();
    *ui = (struct ui_energy) {
        .star = coord_nil(),

        .panel = ui_panel_title(
                make_dim(77 * cell.w, ui_layout_inf),
                ui_str_c("energy")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_energy,
        .parent = ui_view_star,
        .slots = ui_slot_right_sub,
        .panel = ui->panel,
        .fn = {
            .free = ui_energy_free,
            .update_state = ui_energy_update_state,
            .update_frame = ui_energy_update_frame,
            .event = ui_energy_event,
            .render = ui_energy_render,
        },
    };
}

static void ui_energy_free(void *state)
{
    struct ui_energy *ui = state;
    ui_panel_free(ui->panel);
    ui_histo_free(&ui->histo);
    free(ui);
}

void ui_energy_show(struct coord star)
{
    struct ui_energy *ui = ui_state(ui_view_energy);

    ui->star = star;
    if (coord_is_nil(ui->star)) { ui_hide(ui_view_energy); return; }

    ui_energy_update_frame(ui);
    ui_show(ui_view_energy);
}

static void ui_energy_update_state(void *state)
{
    struct ui_energy *ui = state;

    struct chunk *chunk = proxy_chunk(ui->star);
    if (!chunk) return;

    world_ts t = proxy_time();
    if (ui->last_t == t) return;
    ui->last_t = t;

    const struct star *star = chunk_star(chunk);
    const struct energy *energy = chunk_energy(chunk);
    ui_histo_advance(&ui->histo, t);
    ui_histo_push(&ui->histo, ui_energy_consumed, energy->consumed);
    ui_histo_push(&ui->histo, ui_energy_saved, energy->item.battery.stored);
    ui_histo_push(&ui->histo, ui_energy_need, energy->need - energy->consumed);
    ui_histo_push(&ui->histo, ui_energy_stored, energy->item.battery.produced);
    ui_histo_push(&ui->histo, ui_energy_fusion, energy->item.fusion.produced);
    ui_histo_push(&ui->histo, ui_energy_solar, energy_prod_solar(energy, star));
    ui_histo_push(&ui->histo, ui_energy_burner, energy->item.burner);
    ui_histo_push(&ui->histo, ui_energy_kwheel, energy_prod_kwheel(energy, star));
}

static void ui_energy_update_frame(void *state)
{
    struct ui_energy *ui = state;

    struct chunk *chunk = proxy_chunk(ui->star);
    if (!chunk) { ui_energy_show(coord_nil()); return; }

    const struct tech *tech = proxy_tech();
    ui_histo_series(&ui->histo, ui_energy_solar)->visible = tech_known(tech, item_solar);
    ui_histo_series(&ui->histo, ui_energy_burner)->visible = tech_known(tech, item_burner);
    ui_histo_series(&ui->histo, ui_energy_kwheel)->visible = tech_known(tech, item_kwheel);

    ui_histo_update_legend(&ui->histo);
}

static void ui_energy_event(void *state)
{
    struct ui_energy *ui = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ui->star, ev->star))
            ui_energy_show(ev->star);

    ui_histo_event(&ui->histo);
}

static void ui_energy_render(void *state, struct ui_layout *layout)
{
    struct ui_energy *ui = state;
    ui_histo_render(&ui->histo, layout);
}
