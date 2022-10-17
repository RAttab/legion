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
    ui_energy_left_first = 0,

    ui_energy_stored = ui_energy_left_first,
    ui_energy_fusion,
    ui_energy_solar,
    ui_energy_burner,
    ui_energy_kwheel,

    ui_energy_left_last,
    ui_energy_right_first = ui_energy_left_last,

    ui_energy_consumed = ui_energy_right_first,
    ui_energy_saved,
    ui_energy_need,

    ui_energy_right_last,


    ui_energy_len = ui_energy_right_last,
    ui_energy_left_len = ui_energy_left_last  - ui_energy_left_first,
    ui_energy_right_len = ui_energy_right_last  - ui_energy_right_first,
};

struct ui_energy
{
    struct coord star;
    world_ts last_t;
    bool show[ui_energy_len];

    struct ui_panel *panel;

    struct ui_label time, time_val;
    struct ui_label scale;
    struct ui_input scale_val;
    struct ui_button scale_set;

    struct ui_label consumed, consumed_val, consumed_tick;
    struct ui_label saved, saved_val, saved_tick;
    struct ui_label need, need_val, need_tick;

    struct ui_label stored, stored_val, stored_tick;
    struct ui_label fusion, fusion_val, fusion_tick;
    struct ui_label solar, solar_val, solar_tick;
    struct ui_label burner, burner_val, burner_tick;
    struct ui_label kwheel, kwheel_val, kwheel_tick;
    struct ui_label per_tick;

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

        [ui_energy_stored] =   { 0, ui_st.rgba.energy.stored },
        [ui_energy_fusion] =   { 0, ui_st.rgba.energy.fusion },
        [ui_energy_solar] =    { 0, ui_st.rgba.energy.solar },
        [ui_energy_burner] =   { 0, ui_st.rgba.energy.burner },
        [ui_energy_kwheel] =   { 0, ui_st.rgba.energy.kwheel },

        [ui_energy_consumed] = { 1, ui_st.rgba.energy.consumed },
        [ui_energy_saved] =    { 1, ui_st.rgba.energy.saved },
        [ui_energy_need] =     { 1, ui_st.rgba.energy.need },
    };

    struct ui_energy *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_energy) {
        .star = coord_nil(),

        .panel = ui_panel_title(pos, dim, ui_str_c("energy")),

        .time = ui_label_new(ui_str_c("time:     ")),
        .time_val = ui_label_new(ui_str_v(30)),

        .scale = ui_label_new(ui_str_c("t-scale:  ")),
        .scale_val = ui_input_new(8),
        .scale_set = ui_button_new(ui_str_c(">")),

        .consumed = ui_label_new_s(&ui_st.label.energy.consumed, ui_str_c("consumed: ")),
        .consumed_val = ui_label_new(ui_str_v(str_scaled_len)),
        .consumed_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .saved = ui_label_new_s(&ui_st.label.energy.saved, ui_str_c("saved:    ")),
        .saved_val = ui_label_new(ui_str_v(str_scaled_len)),
        .saved_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .need = ui_label_new_s(&ui_st.label.energy.need, ui_str_c("need:     ")),
        .need_val = ui_label_new(ui_str_v(str_scaled_len)),
        .need_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .stored = ui_label_new_s(&ui_st.label.energy.stored, ui_str_c("stored:   ")),
        .stored_val = ui_label_new(ui_str_v(str_scaled_len)),
        .stored_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .fusion = ui_label_new_s(&ui_st.label.energy.fusion, ui_str_c("fusion:   ")),
        .fusion_val = ui_label_new(ui_str_v(str_scaled_len)),
        .fusion_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .solar = ui_label_new_s(&ui_st.label.energy.solar, ui_str_c("solar:    ")),
        .solar_val = ui_label_new(ui_str_v(str_scaled_len)),
        .solar_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .burner = ui_label_new_s(&ui_st.label.energy.burner, ui_str_c("burner:   ")),
        .burner_val = ui_label_new(ui_str_v(str_scaled_len)),
        .burner_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .kwheel = ui_label_new_s(&ui_st.label.energy.kwheel, ui_str_c("k-wheel:  ")),
        .kwheel_val = ui_label_new(ui_str_v(str_scaled_len)),
        .kwheel_tick = ui_label_new(ui_str_v(str_scaled_len)),

        .per_tick = ui_label_new(ui_str_c("/t")),

        .histo = ui_histo_new(
                make_dim(ui_layout_inf, ui_layout_inf),
                series, array_len(series)),
    };

    memset(ui->show, 0, sizeof(ui->show));
    ui->show[ui_energy_consumed] = true;
    ui->show[ui_energy_stored] = true;
    ui->show[ui_energy_need] = true;
    ui->show[ui_energy_saved] = true;
    ui->show[ui_energy_fusion] = true;

    ui_input_set(&ui->scale_val, "1");

    ui_panel_hide(ui->panel);
    return ui;
}

void ui_energy_free(struct ui_energy *ui)
{
    ui_panel_free(ui->panel);

    ui_label_free(&ui->time);
    ui_label_free(&ui->time_val);

    ui_label_free(&ui->scale);
    ui_input_free(&ui->scale_val);
    ui_button_free(&ui->scale_set);

    ui_label_free(&ui->consumed);
    ui_label_free(&ui->consumed_val);
    ui_label_free(&ui->consumed_tick);

    ui_label_free(&ui->saved);
    ui_label_free(&ui->saved_val);
    ui_label_free(&ui->saved_tick);

    ui_label_free(&ui->need);
    ui_label_free(&ui->need_val);
    ui_label_free(&ui->need_tick);

    ui_label_free(&ui->stored);
    ui_label_free(&ui->stored_val);
    ui_label_free(&ui->stored_tick);

    ui_label_free(&ui->fusion);
    ui_label_free(&ui->fusion_val);
    ui_label_free(&ui->fusion_tick);

    ui_label_free(&ui->solar);
    ui_label_free(&ui->solar_val);
    ui_label_free(&ui->solar_tick);

    ui_label_free(&ui->burner);
    ui_label_free(&ui->burner_val);
    ui_label_free(&ui->burner_tick);

    ui_label_free(&ui->kwheel);
    ui_label_free(&ui->kwheel_val);
    ui_label_free(&ui->kwheel_tick);

    ui_label_free(&ui->per_tick);

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
    ui->show[ui_energy_solar] = tech_known(tech, ITEM_SOLAR);
    ui->show[ui_energy_burner] = tech_known(tech, ITEM_BURNER);
    ui->show[ui_energy_kwheel] = tech_known(tech, ITEM_KWHEEL);

    if (!ui->histo.hover.active) {
        ui_set_nil(&ui->time_val);

        ui_set_nil(&ui->consumed_val);
        ui_set_nil(&ui->consumed_tick);

        ui_set_nil(&ui->saved_val);
        ui_set_nil(&ui->saved_tick);

        ui_set_nil(&ui->need_val);
        ui_set_nil(&ui->need_tick);

        ui_set_nil(&ui->stored_val);
        ui_set_nil(&ui->stored_tick);

        ui_set_nil(&ui->fusion_val);
        ui_set_nil(&ui->fusion_tick);

        ui_set_nil(&ui->solar_val);
        ui_set_nil(&ui->solar_tick);

        ui_set_nil(&ui->burner_val);
        ui_set_nil(&ui->burner_tick);

        ui_set_nil(&ui->kwheel_val);
        ui_set_nil(&ui->kwheel_tick);

        ui_set_nil(&ui->stored_val);
        ui_set_nil(&ui->stored_tick);

        return;
    }

    ui_histo_data t = ui->histo.hover.t;
    ui_histo_data scale = ui->histo.hover.row == ui->histo.edge.row ?
        proxy_time(render.proxy) - t : ui->histo.t.scale;
    if (!scale) scale = 1;

    ui_str_setf(ui_set(&ui->time_val), "%lu - %lu", t, t + scale);

    ui_histo_data *data = ui_histo_at(&ui->histo, ui->histo.hover.row);

    ui_str_set_scaled(ui_set(&ui->consumed_val), data[ui_energy_consumed]);
    ui_str_set_scaled(ui_set(&ui->consumed_tick), data[ui_energy_consumed] / scale);

    ui_str_set_scaled(ui_set(&ui->saved_val), data[ui_energy_saved]);
    ui_str_set_scaled(ui_set(&ui->saved_tick), data[ui_energy_saved] / scale);

    ui_str_set_scaled(ui_set(&ui->need_val), data[ui_energy_need]);
    ui_str_set_scaled(ui_set(&ui->need_tick), data[ui_energy_need] / scale);

    ui_str_set_scaled(ui_set(&ui->stored_val), data[ui_energy_stored]);
    ui_str_set_scaled(ui_set(&ui->stored_tick), data[ui_energy_stored] / scale);

    ui_str_set_scaled(ui_set(&ui->fusion_val), data[ui_energy_fusion]);
    ui_str_set_scaled(ui_set(&ui->fusion_tick), data[ui_energy_fusion] / scale);

    ui_str_set_scaled(ui_set(&ui->solar_val), data[ui_energy_solar]);
    ui_str_set_scaled(ui_set(&ui->solar_tick), data[ui_energy_solar] / scale);

    ui_str_set_scaled(ui_set(&ui->burner_val), data[ui_energy_burner]);
    ui_str_set_scaled(ui_set(&ui->burner_tick), data[ui_energy_burner] / scale);

    ui_str_set_scaled(ui_set(&ui->kwheel_val), data[ui_energy_kwheel]);
    ui_str_set_scaled(ui_set(&ui->kwheel_tick), data[ui_energy_kwheel] / scale);
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

    const size_t mid = layout.row.dim.w / 2;

    {
        ui_label_render(&ui->time, &layout, renderer);
        ui_label_render(&ui->time_val, &layout, renderer);

        ui_layout_sep_x(&layout, layout.row.dim.w - mid);

        ui_label_render(&ui->scale, &layout, renderer);
        ui_input_render(&ui->scale_val, &layout, renderer);
        ui_button_render(&ui->scale_set, &layout, renderer);

        ui_layout_next_row(&layout);
    }

    ui_layout_sep_row(&layout);

    size_t left = ui_energy_left_first, right = ui_energy_right_first;
    const size_t rows = legion_max(ui_energy_left_len, ui_energy_right_len);

    for (; left < rows || right < rows; ++left, ++right) {
        bool empty = true;

        if (left < ui_energy_left_last && ui->show[left]) {
            empty = false;

            switch (left)
            {

            case ui_energy_stored: {
                ui_label_render(&ui->stored, &layout, renderer);
                ui_label_render(&ui->stored_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->stored_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_fusion: {
                ui_label_render(&ui->fusion, &layout, renderer);
                ui_label_render(&ui->fusion_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->fusion_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_solar: {
                ui_label_render(&ui->solar, &layout, renderer);
                ui_label_render(&ui->solar_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->solar_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_burner: {
                ui_label_render(&ui->burner, &layout, renderer);
                ui_label_render(&ui->burner_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->burner_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_kwheel: {
                ui_label_render(&ui->kwheel, &layout, renderer);
                ui_label_render(&ui->kwheel_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->kwheel_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            default: { assert(false); }
            }
        }

        ui_layout_sep_x(&layout, layout.row.dim.w - mid);

        if (right < ui_energy_right_last && ui->show[right]) {
            empty = false;

            switch (right)
            {

            case ui_energy_consumed: {
                ui_label_render(&ui->consumed, &layout, renderer);
                ui_label_render(&ui->consumed_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->consumed_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_saved: {
                ui_label_render(&ui->saved, &layout, renderer);
                ui_label_render(&ui->saved_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->saved_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            case ui_energy_need: {
                ui_label_render(&ui->need, &layout, renderer);
                ui_label_render(&ui->need_val, &layout, renderer);
                ui_layout_sep_cols(&layout, 2);
                ui_label_render(&ui->need_tick, &layout, renderer);
                ui_label_render(&ui->per_tick, &layout, renderer);
                ui_layout_sep_col(&layout);
                break;
            }

            default: { assert(false); }
            }

        }

        if (empty) ui_layout_sep_row(&layout);
        ui_layout_next_row(&layout);
    }

    ui_layout_sep_row(&layout);

    ui_histo_render(&ui->histo, &layout, renderer);
}
