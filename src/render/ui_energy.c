/* ui_energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/energy.h"


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


struct ui_energy *ui_energy_new(void)
{
    size_t width = 80 * ui_st.font.dim.w;
    struct pos pos = make_pos(
            render.rect.w - width - ui_star_width(render.ui.star),
            ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

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
    *ui = (struct ui_energy) {
        .star = coord_nil(),
        .panel = ui_panel_title(pos, dim, ui_str_c("energy")),
        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    ui_panel_hide(ui->panel);
    return ui;
}

void ui_energy_free(struct ui_energy *ui)
{
    ui_panel_free(ui->panel);
    ui_histo_free(&ui->histo);
    free(ui);
}

static void ui_energy_clear(struct ui_energy *ui)
{
    ui->star = coord_nil();
    ui_histo_clear(&ui->histo);
    ui_panel_hide(ui->panel);
}

void ui_energy_update_state(struct ui_energy *ui)
{
    if (!ui_panel_is_visible(ui->panel)) return;

    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if (!chunk) return;

    world_ts t = proxy_time(render.proxy);
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

static void ui_energy_update(struct ui_energy *ui)
{
    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if (!chunk) { ui_energy_clear(ui); return; }

    const struct tech *tech = proxy_tech(render.proxy);
    ui_histo_series(&ui->histo, ui_energy_solar)->visible = tech_known(tech, item_solar);
    ui_histo_series(&ui->histo, ui_energy_burner)->visible = tech_known(tech, item_burner);
    ui_histo_series(&ui->histo, ui_energy_kwheel)->visible = tech_known(tech, item_kwheel);

    ui_histo_update_legend(&ui->histo);
}

static bool ui_energy_event_user(struct ui_energy *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(ui->panel)) return false;
        ui_energy_update(ui);
        return false;
    }

    case EV_ENERGY_TOGGLE: {
        if (ui_panel_is_visible(ui->panel)) {
            ui_energy_clear(ui);
            return false;
        }

        ui->star = coord_from_u64((uintptr_t) ev->user.data1);
        assert(!coord_is_nil(ui->star));
        ui_energy_update(ui);
        ui_panel_show(ui->panel);
        return false;
    }

    case EV_STATE_LOAD:
    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_STAR_CLEAR:
    case EV_ITEM_SELECT: { ui_energy_clear(ui); return false; }

    default: { return false; }
    }

}

bool ui_energy_event(struct ui_energy *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_energy_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(ui->panel, ev))) {
        if (ret == ui_consume && !ui_panel_is_visible(ui->panel))
            ui_energy_clear(ui);
        return ret != ui_skip;
    }

    if ((ret = ui_histo_event(&ui->histo, ev))) return true;

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_energy_render(struct ui_energy *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_histo_render(&ui->histo, &layout, renderer);
}
