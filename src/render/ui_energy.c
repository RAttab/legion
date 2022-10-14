/* ui_energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/energy.h"

static void ui_energy_proxy_update(void *);

// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------


enum
{
    ui_energy_consumed = 0,
    ui_energy_solar,
    ui_energy_burner,
    ui_energy_kwheel,
    ui_energy_battery,
    ui_energy_max,
};

struct ui_energy
{
    struct coord star;
    world_ts last_t;
    bool show[ui_energy_max];

    struct ui_panel *panel;

    struct ui_label time, time_val;
    struct ui_label consumed, consumed_val, consumed_tick;
    struct ui_label solar, solar_val, solar_tick;
    struct ui_label burner, burner_val, burner_tick;
    struct ui_label kwheel, kwheel_val, kwheel_tick;
    struct ui_label battery, battery_val, battery_tick;
    struct ui_label per_tick;

    struct ui_label scale;
    struct ui_input scale_val;
    struct ui_button scale_set;

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
        [ui_energy_consumed] = { 0, ui_st.rgba.energy.consumed },
        [ui_energy_solar] =    { 1, ui_st.rgba.energy.solar },
        [ui_energy_burner] =   { 1, ui_st.rgba.energy.burner },
        [ui_energy_kwheel] =   { 1, ui_st.rgba.energy.kwheel },
        [ui_energy_battery] =  { 1, ui_st.rgba.energy.battery },
    };

    struct ui_energy *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_energy) {
        .star = coord_nil(),

        .panel = ui_panel_title(pos, dim, ui_str_c("energy")),

        .time = ui_label_new(ui_str_c("time: ")),
        .time_val = ui_label_new(ui_str_v(30)),

        .consumed = ui_label_new_s(&ui_st.label.energy.consumed, ui_str_c("consumed: ")),
        .consumed_val = ui_label_new(ui_str_v(str_scaled_len)),
        .consumed_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .solar = ui_label_new_s(&ui_st.label.energy.solar, ui_str_c("solar: ")),
        .solar_val = ui_label_new(ui_str_v(str_scaled_len)),
        .solar_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .burner = ui_label_new_s(&ui_st.label.energy.burner, ui_str_c("burner: ")),
        .burner_val = ui_label_new(ui_str_v(str_scaled_len)),
        .burner_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .kwheel = ui_label_new_s(&ui_st.label.energy.kwheel, ui_str_c("k-wheel: ")),
        .kwheel_val = ui_label_new(ui_str_v(str_scaled_len)),
        .kwheel_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .battery = ui_label_new_s(&ui_st.label.energy.battery, ui_str_c("battery: ")),
        .battery_val = ui_label_new(ui_str_v(str_scaled_len)),
        .battery_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .per_tick = ui_label_new(ui_str_c("/t")),

        .scale = ui_label_new(ui_str_c("t-scale: ")),
        .scale_val = ui_input_new(8),
        .scale_set = ui_button_new(ui_str_c(">")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    memset(ui->show, 0, sizeof(ui->show));
    ui->show[ui_energy_consumed] = true;

    ui_input_set(&ui->scale_val, "1");

    proxy_notify(render.proxy, ui_energy_proxy_update, ui);

    ui_panel_hide(ui->panel);
    return ui;
}

void ui_energy_free(struct ui_energy *ui)
{
    ui_panel_free(ui->panel);

    ui_label_free(&ui->solar);
    ui_label_free(&ui->solar_val);
    ui_label_free(&ui->solar_tick);

    ui_label_free(&ui->burner);
    ui_label_free(&ui->burner_val);
    ui_label_free(&ui->burner_tick);

    ui_label_free(&ui->kwheel);
    ui_label_free(&ui->kwheel_val);
    ui_label_free(&ui->kwheel_tick);

    ui_label_free(&ui->battery);
    ui_label_free(&ui->battery_val);
    ui_label_free(&ui->battery_tick);

    ui_label_free(&ui->consumed);
    ui_label_free(&ui->consumed_val);
    ui_label_free(&ui->consumed_tick);

    ui_label_free(&ui->per_tick);

    ui_label_free(&ui->scale);
    ui_input_free(&ui->scale_val);
    ui_button_free(&ui->scale_set);

    ui_histo_free(&ui->histo);

    free(ui);
}

static void ui_energy_clear(struct ui_energy *ui)
{
    ui->star = coord_nil();
    ui_histo_clear(&ui->histo);
    ui_panel_hide(ui->panel);
}

static void ui_energy_update(struct ui_energy *ui)
{
    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if (!chunk) { ui_energy_clear(ui); return; }

    const struct tech *tech = proxy_tech(render.proxy);
    ui->show[ui_energy_solar] = tech_known(tech, ITEM_SOLAR);
    ui->show[ui_energy_burner] = tech_known(tech, ITEM_BURNER);
    ui->show[ui_energy_kwheel] = tech_known(tech, ITEM_KWHEEL);
    ui->show[ui_energy_battery] = tech_known(tech, ITEM_BATTERY);

    if (!ui->histo.hover.active) {
        ui_set_nil(&ui->time_val);
        ui_set_nil(&ui->consumed_val);
        ui_set_nil(&ui->solar_val);
        ui_set_nil(&ui->burner_val);
        ui_set_nil(&ui->kwheel_val);
        ui_set_nil(&ui->battery_val);
        return;
    }

    ui_histo_data t = ui->histo.hover.t;
    ui_histo_data scale = ui->histo.t.scale;
    ui_str_setf(ui_set(&ui->time_val), "%lu - %lu", t, t + scale);

    ui_histo_data *data = ui_histo_at_t(&ui->histo, t);

    ui_str_set_scaled(ui_set(&ui->consumed_val), data[ui_energy_consumed]);
    ui_str_set_scaled(ui_set(&ui->consumed_tick), data[ui_energy_consumed] / scale);

    ui_str_set_scaled(ui_set(&ui->solar_val), data[ui_energy_solar]);
    ui_str_set_scaled(ui_set(&ui->solar_tick), data[ui_energy_solar] / scale);

    ui_str_set_scaled(ui_set(&ui->burner_val), data[ui_energy_burner]);
    ui_str_set_scaled(ui_set(&ui->burner_tick), data[ui_energy_burner] / scale);

    ui_str_set_scaled(ui_set(&ui->kwheel_val), data[ui_energy_kwheel]);
    ui_str_set_scaled(ui_set(&ui->kwheel_tick), data[ui_energy_kwheel] / scale);

    ui_str_set_scaled(ui_set(&ui->battery_val), data[ui_energy_battery]);
    ui_str_set_scaled(ui_set(&ui->battery_tick), data[ui_energy_battery] / scale);
}

static void ui_energy_proxy_update(void *raw)
{
    struct ui_energy *ui = raw;
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
    ui_histo_push(&ui->histo, ui_energy_solar, energy_prod_solar(energy, star));
    ui_histo_push(&ui->histo, ui_energy_burner, energy->item.burner);
    ui_histo_push(&ui->histo, ui_energy_kwheel, energy_prod_kwheel(energy, star));
    ui_histo_push(&ui->histo, ui_energy_battery, energy->item.battery);
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

static void ui_energy_scale_set(struct ui_energy *ui)
{
    uint64_t scale = 0;
    if (ui_input_get_u64(&ui->scale_val, &scale) && scale)
        ui_histo_scale_t(&ui->histo, scale);

    else render_log(st_error, "unable to set the scale to '%.*s'",
            (unsigned) ui->scale_val.buf.len, ui->scale_val.buf.c);
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

    if ((ret = ui_input_event(&ui->scale_val, ev))) {
        if (ret != ui_action) return true;
        ui_energy_scale_set(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->scale_set, ev))) {
        if (ret != ui_action) return true;
        ui_energy_scale_set(ui);
        return true;
    }

    if ((ret = ui_histo_event(&ui->histo, ev))) return true;

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_energy_render(struct ui_energy *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_layout_dir(&layout, ui_layout_left);
    ui_button_render(&ui->scale_set, &layout, renderer);
    ui_input_render(&ui->scale_val, &layout, renderer);
    ui_label_render(&ui->scale, &layout, renderer);
    ui_layout_dir(&layout, ui_layout_right);

    ui_label_render(&ui->time, &layout, renderer);
    ui_label_render(&ui->time_val, &layout, renderer);

    ui_layout_next_row(&layout);

    if (ui->show[ui_energy_consumed]) {
        ui_label_render(&ui->consumed, &layout, renderer);
        ui_label_render(&ui->consumed_val, &layout, renderer);
        ui_layout_sep_cols(&layout, ui->per_tick.str.len + 1);
    }

    if (ui->show[ui_energy_solar]) {
        ui_label_render(&ui->solar, &layout, renderer);
        ui_label_render(&ui->solar_val, &layout, renderer);
        ui_layout_sep_cols(&layout, ui->per_tick.str.len + 1);
    }

    if (ui->show[ui_energy_burner]) {
        ui_label_render(&ui->burner, &layout, renderer);
        ui_label_render(&ui->burner_val, &layout, renderer);
        ui_layout_sep_cols(&layout, ui->per_tick.str.len + 1);
    }

    if (ui->show[ui_energy_kwheel]) {
        ui_label_render(&ui->kwheel, &layout, renderer);
        ui_label_render(&ui->kwheel_val, &layout, renderer);
        ui_layout_sep_cols(&layout, ui->per_tick.str.len + 1);
    }

    if (ui->show[ui_energy_battery]) {
        ui_label_render(&ui->battery, &layout, renderer);
        ui_label_render(&ui->battery_val, &layout, renderer);
        ui_layout_sep_cols(&layout, ui->per_tick.str.len + 1);
    }

    ui_layout_next_row(&layout);

    if (ui->show[ui_energy_consumed]) {
        ui_layout_sep_cols(&layout, ui->consumed.str.len);
        ui_label_render(&ui->consumed_tick, &layout, renderer);
        ui_label_render(&ui->per_tick, &layout, renderer);
        ui_layout_sep_col(&layout);
    }

    if (ui->show[ui_energy_solar]) {
        ui_layout_sep_cols(&layout, ui->solar.str.len);
        ui_label_render(&ui->solar_tick, &layout, renderer);
        ui_label_render(&ui->per_tick, &layout, renderer);
        ui_layout_sep_col(&layout);
    }

    if (ui->show[ui_energy_burner]) {
        ui_layout_sep_cols(&layout, ui->burner.str.len);
        ui_label_render(&ui->burner_tick, &layout, renderer);
        ui_label_render(&ui->per_tick, &layout, renderer);
        ui_layout_sep_col(&layout);
    }

    if (ui->show[ui_energy_kwheel]) {
        ui_layout_sep_cols(&layout, ui->kwheel.str.len);
        ui_label_render(&ui->kwheel_tick, &layout, renderer);
        ui_label_render(&ui->per_tick, &layout, renderer);
        ui_layout_sep_col(&layout);
    }

    if (ui->show[ui_energy_battery]) {
        ui_layout_sep_cols(&layout, ui->battery.str.len);
        ui_label_render(&ui->battery_tick, &layout, renderer);
        ui_label_render(&ui->per_tick, &layout, renderer);
        ui_layout_sep_col(&layout);
    }

    ui_layout_next_row(&layout);
    ui_layout_sep_row(&layout);

    ui_histo_render(&ui->histo, &layout, renderer);
}
