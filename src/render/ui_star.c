/* ui_star.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/energy.h"
#include "db/items.h"
#include "db/specs.h"
#include "utils/vec.h"

static void ui_star_free(void *);
static void ui_star_update(void *);
static bool ui_star_event(void *, SDL_Event *);
static void ui_star_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ui_star_workers
{
    struct ui_button toggle;
    struct ui_label count, count_val;
    struct ui_label queue, queue_val;
    struct ui_label idle, idle_val;
    struct ui_label fail, fail_val;
    struct ui_label clean, clean_val;
};

struct ui_star_energy
{
    bool show;
    struct ui_label name, count;
    struct ui_label prod, prod_val;
    struct ui_label total, total_val;
};

struct ui_star
{
    struct coord id;
    struct star star;

    struct ui_panel *panel;

    struct ui_button goto_factory, goto_map, goto_log;
    struct ui_label name, name_val;
    struct ui_label coord;
    struct ui_link coord_val;

    struct ui_label energy, energy_val;
    struct ui_label elem, elem_val;
    struct ui_tabs tabs;

    struct ui_tree control_list, factory_list;

    struct {
        struct ui_button toggle;
        struct ui_label count, count_val;
    } pills;

    struct ui_star_workers workers;

    struct ui_button energy_toggle;
    struct ui_label need, need_val;
    struct ui_label consumed, consumed_val;
    struct ui_label produced, produced_val;
    struct ui_label stored, stored_val;
    struct ui_star_energy fusion;
    struct ui_star_energy solar;
    struct ui_star_energy kwheel;
    struct ui_star_energy burner;
    struct ui_star_energy battery;

};

static const char *ui_star_elems[] = {
    "A:", "B:", "C:", "D:", "E:",
    "F:", "G:", "H:", "I:", "J:",
    "K:"
};

enum
{
    ui_star_elems_col_len = 2 + str_scaled_len,
    ui_star_elems_total_len = ui_star_elems_col_len * 5 + 4,
};

enum ui_star_tabs { ui_star_control = 1, ui_star_factory, ui_star_logistics, };

void ui_star_alloc(struct ui_view_state *state)
{
    struct ui_star *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_star) {
        .panel = ui_panel_title(
                make_dim(38 * ui_st.font.dim.w, ui_layout_inf),
                ui_str_c("star")),

        .goto_map = ui_button_new(ui_str_c("<< map")),
        .goto_factory = ui_button_new(ui_str_c("<< factory")),
        .goto_log = ui_button_new(ui_str_c("<< log")),

        .name = ui_label_new(ui_str_c("name:  ")),
        .name_val = ui_label_new(ui_str_v(symbol_cap)),

        .coord = ui_label_new(ui_str_c("coord: ")),
        .coord_val = ui_link_new(ui_str_v(coord_str_len)),

        .energy = ui_label_new(ui_str_c("energy: ")),
        .energy_val = ui_label_new(ui_str_v(str_scaled_len)),

        .elem = ui_label_new(ui_str_c(ui_star_elems[0])),
        .elem_val = ui_label_new(ui_str_v(str_scaled_len)),

        .tabs = ui_tabs_new(9, false),

        .control_list = ui_tree_new(
                make_dim(ui_layout_inf, ui_layout_inf), im_id_str_len),
        .factory_list = ui_tree_new(
                make_dim(ui_layout_inf, ui_layout_inf), im_id_str_len),

        .pills = {
            .toggle = ui_button_new(ui_str_c("pills")),
            .count = ui_label_new(ui_str_c("- count: ")),
            .count_val = ui_label_new(ui_str_v(4)),
        },

        .workers = (struct ui_star_workers) {
            .toggle = ui_button_new(ui_str_c("workers")),
            .count = ui_label_new(ui_str_c("- count: ")),
            .count_val = ui_label_new(ui_str_v(3)),
            .queue = ui_label_new(ui_str_c("- queue: ")),
            .queue_val = ui_label_new(ui_str_v(3)),
            .idle = ui_label_new(ui_str_c("- idle:  ")),
            .idle_val = ui_label_new(ui_str_v(3)),
            .fail = ui_label_new(ui_str_c("- fail:  ")),
            .fail_val = ui_label_new(ui_str_v(3)),
            .clean = ui_label_new(ui_str_c("- clean: ")),
            .clean_val = ui_label_new(ui_str_v(3)),
        },

        .energy_toggle = ui_button_new(ui_str_c("energy")),
        .need = ui_label_new(ui_str_c("- need:       ")),
        .need_val = ui_label_new(ui_str_v(str_scaled_len)),
        .consumed = ui_label_new(ui_str_c("- consumed:   ")),
        .consumed_val = ui_label_new(ui_str_v(str_scaled_len)),
        .produced = ui_label_new(ui_str_c("- produced:   ")),
        .produced_val = ui_label_new(ui_str_v(str_scaled_len)),
        .stored = ui_label_new(ui_str_c("- stored:     ")),
        .stored_val = ui_label_new(ui_str_v(str_scaled_len)),

        .fusion = (struct ui_star_energy) {
            .name = ui_label_new(ui_str_c("fusion:       ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .solar = (struct ui_star_energy) {
            .name = ui_label_new(ui_str_c("solar:        ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .burner = (struct ui_star_energy) {
            .name = ui_label_new(ui_str_c("burner:       ")),
            .count = ui_label_new(ui_str_v(3)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .kwheel = (struct ui_star_energy) {
            .name = ui_label_new(ui_str_c("k-wheel:      ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .battery = (struct ui_star_energy) {
            .name = ui_label_new(ui_str_c("battery:      ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- capacity:   ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },
    };

    size_t goto_width = (ui->panel->w.dim.w - ui_st.font.dim.w) / 3;
    ui->goto_map.w.dim.w = goto_width;
    ui->goto_factory.w.dim.w = goto_width;
    ui->goto_log.w.dim.w = goto_width;

    ui_str_setc(ui_tabs_add(&ui->tabs, ui_star_control), "control");
    ui_str_setc(ui_tabs_add(&ui->tabs, ui_star_factory), "factory");
    ui_str_setc(ui_tabs_add(&ui->tabs, ui_star_logistics), "logistics");
    ui_tabs_select(&ui->tabs, ui_star_control);

    ui->pills.toggle.w.dim.w = ui_layout_inf;
    ui->workers.toggle.w.dim.w = ui_layout_inf;
    ui->energy_toggle.w.dim.w = ui_layout_inf;

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_star,
        .slots = ui_slot_right,
        .panel = ui->panel,
        .fn = {
            .free = ui_star_free,
            .update_frame = ui_star_update,
            .event = ui_star_event,
            .render = ui_star_render,
        },
    };
}

static void ui_star_free(void *state)
{
    struct ui_star *ui = state;

    ui_panel_free(ui->panel);

    ui_button_free(&ui->goto_map);
    ui_button_free(&ui->goto_factory);
    ui_button_free(&ui->goto_log);

    ui_label_free(&ui->name);
    ui_label_free(&ui->name_val);

    ui_label_free(&ui->coord);
    ui_link_free(&ui->coord_val);

    ui_label_free(&ui->energy);
    ui_label_free(&ui->energy_val);

    ui_label_free(&ui->elem);
    ui_label_free(&ui->elem_val);

    ui_tabs_free(&ui->tabs);

    ui_tree_free(&ui->control_list);
    ui_tree_free(&ui->factory_list);

    ui_button_free(&ui->pills.toggle);
    ui_label_free(&ui->pills.count);
    ui_label_free(&ui->pills.count_val);

    ui_button_free(&ui->workers.toggle);
    ui_label_free(&ui->workers.count);
    ui_label_free(&ui->workers.count_val);
    ui_label_free(&ui->workers.queue);
    ui_label_free(&ui->workers.queue_val);
    ui_label_free(&ui->workers.idle);
    ui_label_free(&ui->workers.idle_val);
    ui_label_free(&ui->workers.fail);
    ui_label_free(&ui->workers.fail_val);
    ui_label_free(&ui->workers.clean);
    ui_label_free(&ui->workers.clean_val);

    ui_button_free(&ui->energy_toggle);
    ui_label_free(&ui->need);
    ui_label_free(&ui->need_val);
    ui_label_free(&ui->consumed);
    ui_label_free(&ui->consumed_val);
    ui_label_free(&ui->produced);
    ui_label_free(&ui->produced_val);
    ui_label_free(&ui->stored);
    ui_label_free(&ui->stored_val);

    ui_label_free(&ui->fusion.name);
    ui_label_free(&ui->fusion.count);
    ui_label_free(&ui->fusion.prod);
    ui_label_free(&ui->fusion.prod_val);
    ui_label_free(&ui->fusion.total);
    ui_label_free(&ui->fusion.total_val);

    ui_label_free(&ui->solar.name);
    ui_label_free(&ui->solar.count);
    ui_label_free(&ui->solar.prod);
    ui_label_free(&ui->solar.prod_val);
    ui_label_free(&ui->solar.total);
    ui_label_free(&ui->solar.total_val);

    ui_label_free(&ui->burner.name);
    ui_label_free(&ui->burner.count);
    ui_label_free(&ui->burner.prod);
    ui_label_free(&ui->burner.prod_val);
    ui_label_free(&ui->burner.total);
    ui_label_free(&ui->burner.total_val);

    ui_label_free(&ui->kwheel.name);
    ui_label_free(&ui->kwheel.count);
    ui_label_free(&ui->kwheel.prod);
    ui_label_free(&ui->kwheel.prod_val);
    ui_label_free(&ui->kwheel.total);
    ui_label_free(&ui->kwheel.total_val);

    ui_label_free(&ui->battery.name);
    ui_label_free(&ui->battery.count);
    ui_label_free(&ui->battery.prod);
    ui_label_free(&ui->battery.prod_val);
    ui_label_free(&ui->battery.total);
    ui_label_free(&ui->battery.total_val);

    free(ui);
}

void ui_star_show(struct coord star)
{
    struct ui_star *ui = ui_state(ui_view_star);

    struct coord old = legion_xchg(&ui->id, star);
    if (coord_is_nil(ui->id)) { ui_hide(ui_view_star); return; }

    ui_star_update(ui);
    ui_show(ui_view_star);
    if (!coord_eq(old, star))
        render_push_event(ev_star_select, coord_to_u64(star), 0);
}

static void ui_star_update_list(
        struct chunk *chunk, struct ui_tree *tree, im_list filter)
{
    ui_tree_reset(tree);

    enum item item = item_nil;
    ui_node parent = ui_node_nil;
    struct vec16 *ids = chunk_list_filter(chunk, filter);

    for (size_t i = 0; i < ids->len; ++i) {
        im_id id = ids->vals[i];

        if (item != im_id_item(id)) {
            item = im_id_item(id);
            parent = ui_tree_index(tree);
            ui_str_set_item(ui_tree_add(tree, ui_node_nil, make_im_id(item, 0)), item);
        }

        ui_str_set_id(ui_tree_add(tree, parent, id), id);
    }

    vec16_free(ids);
}

static void ui_star_update(void *state)
{
    struct ui_star *ui = state;

    ui_str_set_coord(&ui->coord_val.str, ui->id);

    struct chunk *chunk = proxy_chunk(ui->id);
    if (!chunk) {
        const struct star *star = proxy_star_at(ui->id);
        assert(star);
        ui->star = *star;

        {
            world_seed seed = proxy_seed();
            struct atoms *atoms = proxy_atoms();

            struct symbol sym = {0};
            vm_word name = star_name(ui->id, seed, atoms);
            bool ok = atoms_str(atoms, name, &sym);
            ui_str_set_symbol(&ui->name_val.str, &sym);
            assert(ok);
        }

        ui_tree_clear(&ui->control_list);
        ui_tree_reset(&ui->control_list);
        ui_tree_clear(&ui->factory_list);
        ui_tree_reset(&ui->factory_list);

        ui_str_set_u64(&ui->pills.count_val.str, 0);

        ui_str_set_u64(&ui->workers.count_val.str, 0);
        ui_str_set_u64(&ui->workers.idle_val.str, 0);
        ui_str_set_u64(&ui->workers.fail_val.str, 0);
        ui_str_set_u64(&ui->workers.queue_val.str, 0);

        ui_str_set_scaled(&ui->need_val.str, 0);
        ui_str_set_scaled(&ui->consumed_val.str, 0);
        ui_str_set_scaled(&ui->produced_val.str, 0);
        ui_str_set_scaled(&ui->stored_val.str, 0);

        ui->fusion.show = false;
        ui->solar.show = false;
        ui->burner.show = false;
        ui->kwheel.show = false;
        ui->battery.show = false;
        return;
    }

    {
        struct symbol sym = {0};
        vm_word name = chunk_name(chunk);
        if (atoms_str(proxy_atoms(), name, &sym))
            ui_str_set_symbol(&ui->name_val.str, &sym);
        else ui_str_set_hex(&ui->name_val.str, name);
    }

    ui->star = *chunk_star(chunk);
    ui_star_update_list(chunk, &ui->control_list, im_list_control);
    ui_star_update_list(chunk, &ui->factory_list, im_list_factory);

    ui_str_set_u64(&ui->pills.count_val.str, chunk_scan(chunk, item_pill));

    {
        struct workers workers = chunk_workers(chunk);
        ui_str_set_u64(&ui->workers.count_val.str, workers.count);
        ui_str_set_u64(&ui->workers.queue_val.str, workers.queue);
        ui_str_set_u64(&ui->workers.idle_val.str, workers.idle);
        ui_str_set_u64(&ui->workers.fail_val.str, workers.fail);
        ui_str_set_u64(&ui->workers.clean_val.str, workers.clean);
    }

    {
        const struct tech *tech = proxy_tech();
        struct energy energy = *chunk_energy(chunk);

        ui_str_set_scaled(&ui->need_val.str, energy.need);
        ui_str_set_scaled(&ui->consumed_val.str, energy.consumed);
        ui_str_set_scaled(&ui->produced_val.str, energy.produced);
        ui_str_set_scaled(&ui->stored_val.str, energy.item.battery.stored);

        ui->fusion.show = tech_known(tech, item_fusion);
        ui_str_set_u64(&ui->fusion.count.str, chunk_scan(chunk, item_fusion));
        ui_str_set_scaled(&ui->fusion.total_val.str, energy.item.fusion.produced);
        ui_str_set_scaled(&ui->fusion.prod_val.str, im_fusion_energy_output);

        ui->solar.show = tech_known(tech, item_solar);
        ui_str_set_u64(&ui->solar.count.str, energy.solar);
        ui_str_set_scaled(&ui->solar.total_val.str, energy_prod_solar(&energy, &ui->star));
        energy.solar = 1;
        ui_str_set_scaled(&ui->solar.prod_val.str, energy_prod_solar(&energy, &ui->star));

        ui->burner.show = tech_known(tech, item_burner);
        ui_str_set_u64(&ui->burner.count.str, chunk_scan(chunk, item_burner));
        ui_str_set_scaled(&ui->burner.total_val.str, energy.item.burner);

        ui->kwheel.show = tech_known(tech, item_kwheel);
        ui_str_set_u64(&ui->kwheel.count.str, energy.kwheel);
        ui_str_set_scaled(&ui->kwheel.total_val.str, energy_prod_kwheel(&energy, &ui->star));
        energy.kwheel = 1;
        ui_str_set_scaled(&ui->kwheel.prod_val.str, energy_prod_kwheel(&energy, &ui->star));

        ui->battery.show = tech_known(tech, item_battery);
        ui_str_set_u64(&ui->battery.count.str, energy.battery);
        ui_str_set_scaled(&ui->battery.total_val.str, energy_battery_cap(&energy));
        ui_str_set_scaled(&ui->battery.prod_val.str, im_battery_storage_cap);
    }
}

static void ui_star_event_user(struct ui_star *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_item_select: {
        struct coord coord = coord_from_u64((uintptr_t) ev->user.data2);
        if (!coord_eq(coord, ui->id))
            ui_star_show(coord);

        im_id selected = (uintptr_t) ev->user.data1;
        ui_tree_select(&ui->control_list, selected);
        ui_tree_select(&ui->factory_list, selected);
        return;
    }

    default: { return; }
    }
}


static bool ui_star_event(void *state, SDL_Event *ev)
{
    struct ui_star *ui = state;

    if (ev->type == render.event)
        ui_star_event_user(ui, ev);

    enum ui_ret ret = ui_nil;
    if ((ret = ui_button_event(&ui->goto_map, ev))) {
        if (ret != ui_action) return true;
        ui_map_show(ui->id);
        return true;
    }

    if ((ret = ui_button_event(&ui->goto_factory, ev))) {
        if (ret != ui_action) return true;
        ui_factory_show(ui->id, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->goto_log, ev))) {
        if (ret != ui_action) return true;
        ui_log_show(ui->id);
        return true;
    }

    if ((ret = ui_link_event(&ui->coord_val, ev))) {
        if (ret != ui_action) return true;
        ui_clipboard_copy_hex(coord_to_u64(ui->star.coord));
        return true;
    }

    if ((ret = ui_tabs_event(&ui->tabs, ev))) return ret;

    switch (ui_tabs_selected(&ui->tabs))
    {

    case ui_star_control: {
        if ((ret = ui_tree_event(&ui->control_list, ev))) {
            if (ret != ui_action) return true;
            im_id id = ui->control_list.selected;
            if (ret == ui_action && im_id_seq(id))
                ui_item_show(id, ui->id);
            return true;
        }
        break;
    }

    case ui_star_factory: {
        if ((ret = ui_tree_event(&ui->factory_list, ev))) {
            if (ret != ui_action) return true;
            im_id id = ui->factory_list.selected;
            if (ret == ui_action && im_id_seq(id))
                ui_item_show(id, ui->id);
            return true;
        }
        break;
    }

    case ui_star_logistics: {
        if ((ret = ui_button_event(&ui->pills.toggle, ev))) {
            if (ret != ui_action) return true;
            ui_pills_show(ui->id);
            return true;
        }

        if ((ret = ui_button_event(&ui->workers.toggle, ev))) {
            if (ret != ui_action) return true;
            ui_workers_show(ui->id);
            return true;
        }

        if ((ret = ui_button_event(&ui->energy_toggle, ev))) {
            if (ret != ui_action) return true;
            ui_energy_show(ui->id);
            return true;
        }
        break;
    }

    default: { assert(false); }
    }

    return false;
}

static void ui_star_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_star *ui = state;

    ui_button_render(&ui->goto_log, layout, renderer);
    ui_button_render(&ui->goto_map, layout, renderer);
    ui_button_render(&ui->goto_factory, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->name, layout, renderer);
    ui_label_render(&ui->name_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->coord, layout, renderer);
    ui_link_render(&ui->coord_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    {
        uint32_t energy = ui->star.energy;
        ui_str_set_scaled(&ui->energy_val.str, energy);
        ui->energy_val.s.fg = rgba_gray(0x11 * u64_log2(energy));

        ui_label_render(&ui->energy, layout, renderer);
        ui_label_render(&ui->energy_val, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < items_natural_len; ++i) {
            if (i == items_natural_len-1)
                ui_layout_sep_x(layout,
                        (ui_star_elems_col_len+1)*2 * ui_st.font.dim.w);

            ui_str_setc(&ui->elem.str, ui_star_elems[i]);
            ui_label_render(&ui->elem, layout, renderer);

            uint16_t value = ui->star.elems[i];
            ui_str_set_scaled(&ui->elem_val.str, value);
            ui->elem_val.s.fg = rgba_gray(0x11 * u64_log2(value));
            ui_label_render(&ui->elem_val, layout, renderer);

            size_t col = i % 5;
            if (col < 4) ui_layout_sep_col(layout);
            else ui_layout_next_row(layout);
        }

        ui_layout_next_row(layout);
        ui_layout_sep_row(layout);
    }

    ui_tabs_render(&ui->tabs, layout, renderer);
    ui_layout_next_row(layout);

    switch (ui_tabs_selected(&ui->tabs))
    {

    case ui_star_control: {
        ui_tree_render(&ui->control_list, layout, renderer);
        break;
    }

    case ui_star_factory: {
        ui_tree_render(&ui->factory_list, layout, renderer);
        break;
    }

    case ui_star_logistics: {
        ui_button_render(&ui->pills.toggle, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->pills.count, layout, renderer);
        ui_label_render(&ui->pills.count_val, layout, renderer);
        ui_layout_next_row(layout);

        ui_layout_sep_row(layout);

        ui_button_render(&ui->workers.toggle, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->workers.count, layout, renderer);
        ui_label_render(&ui->workers.count_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->workers.queue, layout, renderer);
        ui_label_render(&ui->workers.queue_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->workers.idle, layout, renderer);
        ui_label_render(&ui->workers.idle_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->workers.fail, layout, renderer);
        ui_label_render(&ui->workers.fail_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->workers.clean, layout, renderer);
        ui_label_render(&ui->workers.clean_val, layout, renderer);
        ui_layout_next_row(layout);

        ui_layout_sep_row(layout);

        ui_button_render(&ui->energy_toggle, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->need, layout, renderer);
        ui_label_render(&ui->need_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->consumed, layout, renderer);
        ui_label_render(&ui->consumed_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->produced, layout, renderer);
        ui_label_render(&ui->produced_val, layout, renderer);
        ui_layout_next_row(layout);
        ui_label_render(&ui->stored, layout, renderer);
        ui_label_render(&ui->stored_val, layout, renderer);
        ui_layout_next_row(layout);

        if (ui->fusion.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ui->fusion.name, layout, renderer);
            ui_label_render(&ui->fusion.count, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->fusion.prod, layout, renderer);
            ui_label_render(&ui->fusion.prod_val, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->fusion.total, layout, renderer);
            ui_label_render(&ui->fusion.total_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        if (ui->solar.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ui->solar.name, layout, renderer);
            ui_label_render(&ui->solar.count, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->solar.prod, layout, renderer);
            ui_label_render(&ui->solar.prod_val, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->solar.total, layout, renderer);
            ui_label_render(&ui->solar.total_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        if (ui->burner.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ui->burner.name, layout, renderer);
            ui_label_render(&ui->burner.count, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->burner.total, layout, renderer);
            ui_label_render(&ui->burner.total_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        if (ui->kwheel.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ui->kwheel.name, layout, renderer);
            ui_label_render(&ui->kwheel.count, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->kwheel.prod, layout, renderer);
            ui_label_render(&ui->kwheel.prod_val, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->kwheel.total, layout, renderer);
            ui_label_render(&ui->kwheel.total_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        if (ui->battery.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ui->battery.name, layout, renderer);
            ui_label_render(&ui->battery.count, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->battery.prod, layout, renderer);
            ui_label_render(&ui->battery.prod_val, layout, renderer);
            ui_layout_next_row(layout);
            ui_label_render(&ui->battery.total, layout, renderer);
            ui_label_render(&ui->battery.total_val, layout, renderer);
            ui_layout_next_row(layout);
        }

        break;
    }

    default: { assert(false); }
    }
}
