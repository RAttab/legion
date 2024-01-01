/* ux_star.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

static void ux_star_free(void *);
static void ux_star_update(void *);
static void ux_star_event(void *);
static void ux_star_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ux_star_workers
{
    struct ui_button toggle;
    struct ui_label count, count_val;
    struct ui_label queue, queue_val;
    struct ui_label idle, idle_val;
    struct ui_label fail, fail_val;
    struct ui_label clean, clean_val;
};

struct ux_star_energy
{
    bool show;
    struct ui_label name, count;
    struct ui_label prod, prod_val;
    struct ui_label total, total_val;
};

struct ux_star
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

    struct ux_star_workers workers;

    struct ui_button energy_toggle;
    struct ui_label need, need_val;
    struct ui_label consumed, consumed_val;
    struct ui_label produced, produced_val;
    struct ui_label stored, stored_val;
    struct ux_star_energy fusion;
    struct ux_star_energy solar;
    struct ux_star_energy kwheel;
    struct ux_star_energy burner;
    struct ux_star_energy battery;

};

static const char *ux_star_elems[] = {
    "A:", "B:", "C:", "D:", "E:",
    "F:", "G:", "H:", "I:", "J:",
    "K:"
};

constexpr size_t ux_star_elems_col_len = 2 + str_scaled_len;
constexpr size_t ux_star_elems_total_len = ux_star_elems_col_len * 5 + 4;

enum ux_star_tabs { ux_star_control = 1, ux_star_factory, ux_star_logistics, };

void ux_star_alloc(struct ux_view_state *state)
{
    struct ux_star *ux = calloc(1, sizeof(*ux));

    struct dim cell = engine_cell();
    *ux = (struct ux_star) {
        .panel = ui_panel_title(
                make_dim(38 * cell.w, ui_layout_inf),
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

        .elem = ui_label_new(ui_str_c(ux_star_elems[0])),
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

        .workers = (struct ux_star_workers) {
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

        .fusion = (struct ux_star_energy) {
            .name = ui_label_new(ui_str_c("fusion:       ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .solar = (struct ux_star_energy) {
            .name = ui_label_new(ui_str_c("solar:        ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .burner = (struct ux_star_energy) {
            .name = ui_label_new(ui_str_c("burner:       ")),
            .count = ui_label_new(ui_str_v(3)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .kwheel = (struct ux_star_energy) {
            .name = ui_label_new(ui_str_c("k-wheel:      ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- production: ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },

        .battery = (struct ux_star_energy) {
            .name = ui_label_new(ui_str_c("battery:      ")),
            .count = ui_label_new(ui_str_v(3)),
            .prod = ui_label_new(ui_str_c("- capacity:   ")),
            .prod_val = ui_label_new(ui_str_v(str_scaled_len)),
            .total = ui_label_new(ui_str_c("- total:      ")),
            .total_val = ui_label_new(ui_str_v(str_scaled_len)),
        },
    };

    size_t goto_width = (ux->panel->w.w - cell.w) / 3;
    ux->goto_map.w.w = goto_width;
    ux->goto_factory.w.w = goto_width;
    ux->goto_log.w.w = goto_width;

    ui_str_setc(ui_tabs_add(&ux->tabs, ux_star_control), "control");
    ui_str_setc(ui_tabs_add(&ux->tabs, ux_star_factory), "factory");
    ui_str_setc(ui_tabs_add(&ux->tabs, ux_star_logistics), "logistics");
    ui_tabs_select(&ux->tabs, ux_star_control);

    ux->pills.toggle.w.w = ui_layout_inf;
    ux->workers.toggle.w.w = ui_layout_inf;
    ux->energy_toggle.w.w = ui_layout_inf;

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_star,
        .slots = ux_slot_right,
        .panel = ux->panel,
        .fn = {
            .free = ux_star_free,
            .update = ux_star_update,
            .event = ux_star_event,
            .render = ux_star_render,
        },
    };
}

static void ux_star_free(void *state)
{
    struct ux_star *ux = state;

    ui_panel_free(ux->panel);

    ui_button_free(&ux->goto_map);
    ui_button_free(&ux->goto_factory);
    ui_button_free(&ux->goto_log);

    ui_label_free(&ux->name);
    ui_label_free(&ux->name_val);

    ui_label_free(&ux->coord);
    ui_link_free(&ux->coord_val);

    ui_label_free(&ux->energy);
    ui_label_free(&ux->energy_val);

    ui_label_free(&ux->elem);
    ui_label_free(&ux->elem_val);

    ui_tabs_free(&ux->tabs);

    ui_tree_free(&ux->control_list);
    ui_tree_free(&ux->factory_list);

    ui_button_free(&ux->pills.toggle);
    ui_label_free(&ux->pills.count);
    ui_label_free(&ux->pills.count_val);

    ui_button_free(&ux->workers.toggle);
    ui_label_free(&ux->workers.count);
    ui_label_free(&ux->workers.count_val);
    ui_label_free(&ux->workers.queue);
    ui_label_free(&ux->workers.queue_val);
    ui_label_free(&ux->workers.idle);
    ui_label_free(&ux->workers.idle_val);
    ui_label_free(&ux->workers.fail);
    ui_label_free(&ux->workers.fail_val);
    ui_label_free(&ux->workers.clean);
    ui_label_free(&ux->workers.clean_val);

    ui_button_free(&ux->energy_toggle);
    ui_label_free(&ux->need);
    ui_label_free(&ux->need_val);
    ui_label_free(&ux->consumed);
    ui_label_free(&ux->consumed_val);
    ui_label_free(&ux->produced);
    ui_label_free(&ux->produced_val);
    ui_label_free(&ux->stored);
    ui_label_free(&ux->stored_val);

    ui_label_free(&ux->fusion.name);
    ui_label_free(&ux->fusion.count);
    ui_label_free(&ux->fusion.prod);
    ui_label_free(&ux->fusion.prod_val);
    ui_label_free(&ux->fusion.total);
    ui_label_free(&ux->fusion.total_val);

    ui_label_free(&ux->solar.name);
    ui_label_free(&ux->solar.count);
    ui_label_free(&ux->solar.prod);
    ui_label_free(&ux->solar.prod_val);
    ui_label_free(&ux->solar.total);
    ui_label_free(&ux->solar.total_val);

    ui_label_free(&ux->burner.name);
    ui_label_free(&ux->burner.count);
    ui_label_free(&ux->burner.prod);
    ui_label_free(&ux->burner.prod_val);
    ui_label_free(&ux->burner.total);
    ui_label_free(&ux->burner.total_val);

    ui_label_free(&ux->kwheel.name);
    ui_label_free(&ux->kwheel.count);
    ui_label_free(&ux->kwheel.prod);
    ui_label_free(&ux->kwheel.prod_val);
    ui_label_free(&ux->kwheel.total);
    ui_label_free(&ux->kwheel.total_val);

    ui_label_free(&ux->battery.name);
    ui_label_free(&ux->battery.count);
    ui_label_free(&ux->battery.prod);
    ui_label_free(&ux->battery.prod_val);
    ui_label_free(&ux->battery.total);
    ui_label_free(&ux->battery.total_val);

    free(ux);
}

void ux_star_show(struct coord star)
{
    struct ux_star *ux = ux_state(ux_view_star);

    struct coord old = legion_xchg(&ux->id, star);
    if (coord_is_nil(ux->id)) { ux_hide(ux_view_star); return; }

    ux_star_update(ux);
    ux_show(ux_view_star);
    if (!coord_eq(old, star)) ev_set_select_star(star);
}

static void ux_star_update_list(
        struct chunk *chunk, struct ui_tree *tree, im_list filter)
{
    ui_tree_reset(tree);

    enum item item = item_nil;
    ui_tree_node parent = ui_tree_node_nil;
    struct vec16 *ids = chunk_list_filter(chunk, filter);

    for (size_t i = 0; i < ids->len; ++i) {
        im_id id = ids->vals[i];

        if (item != im_id_item(id)) {
            item = im_id_item(id);
            parent = ui_tree_index(tree);
            ui_str_set_item(ui_tree_add(tree, ui_tree_node_nil, make_im_id(item, 0)), item);
        }

        ui_str_set_id(ui_tree_add(tree, parent, id), id);
    }

    vec16_free(ids);
}

static void ux_star_update(void *state)
{
    struct ux_star *ux = state;

    ui_str_set_coord(&ux->coord_val.str, ux->id);

    struct chunk *chunk = proxy_chunk(ux->id);
    if (!chunk) {
        const struct star *star = proxy_star_at(ux->id);
        assert(star);
        ux->star = *star;

        {
            world_seed seed = proxy_seed();
            struct atoms *atoms = proxy_atoms();

            struct symbol sym = {0};
            vm_word name = star_name(ux->id, seed, atoms);
            bool ok = atoms_str(atoms, name, &sym);
            ui_str_set_symbol(&ux->name_val.str, &sym);
            assert(ok);
        }

        ui_tree_clear(&ux->control_list);
        ui_tree_reset(&ux->control_list);
        ui_tree_clear(&ux->factory_list);
        ui_tree_reset(&ux->factory_list);

        ui_str_set_u64(&ux->pills.count_val.str, 0);

        ui_str_set_u64(&ux->workers.count_val.str, 0);
        ui_str_set_u64(&ux->workers.idle_val.str, 0);
        ui_str_set_u64(&ux->workers.fail_val.str, 0);
        ui_str_set_u64(&ux->workers.queue_val.str, 0);

        ui_str_set_scaled(&ux->need_val.str, 0);
        ui_str_set_scaled(&ux->consumed_val.str, 0);
        ui_str_set_scaled(&ux->produced_val.str, 0);
        ui_str_set_scaled(&ux->stored_val.str, 0);

        ux->fusion.show = false;
        ux->solar.show = false;
        ux->burner.show = false;
        ux->kwheel.show = false;
        ux->battery.show = false;
        return;
    }

    {
        struct symbol sym = {0};
        vm_word name = chunk_name(chunk);
        if (atoms_str(proxy_atoms(), name, &sym))
            ui_str_set_symbol(&ux->name_val.str, &sym);
        else ui_str_set_hex(&ux->name_val.str, name);
    }

    ux->star = *chunk_star(chunk);
    ux_star_update_list(chunk, &ux->control_list, im_list_control);
    ux_star_update_list(chunk, &ux->factory_list, im_list_factory);

    ui_str_set_u64(&ux->pills.count_val.str, chunk_count(chunk, item_pill));

    {
        const struct workers *workers = chunk_workers(chunk);
        ui_str_set_u64(&ux->workers.count_val.str, workers->count);
        ui_str_set_u64(&ux->workers.queue_val.str, workers->queue);
        ui_str_set_u64(&ux->workers.idle_val.str, workers->idle);
        ui_str_set_u64(&ux->workers.fail_val.str, workers->fail);
        ui_str_set_u64(&ux->workers.clean_val.str, workers->clean);
    }

    {
        const struct tech *tech = proxy_tech();
        struct energy energy = *chunk_energy(chunk);

        ui_str_set_scaled(&ux->need_val.str, energy.need);
        ui_str_set_scaled(&ux->consumed_val.str, energy.consumed);
        ui_str_set_scaled(&ux->produced_val.str, energy.produced);
        ui_str_set_scaled(&ux->stored_val.str, energy.item.battery.stored);

        ux->fusion.show = tech_known(tech, item_fusion);
        ui_str_set_u64(&ux->fusion.count.str, chunk_count(chunk, item_fusion));
        ui_str_set_scaled(&ux->fusion.total_val.str, energy.item.fusion.produced);
        ui_str_set_scaled(&ux->fusion.prod_val.str, im_fusion_energy_output);

        ux->solar.show = tech_known(tech, item_solar);
        ui_str_set_u64(&ux->solar.count.str, energy.solar);
        ui_str_set_scaled(&ux->solar.total_val.str, energy_prod_solar(&energy, &ux->star));
        energy.solar = 1;
        ui_str_set_scaled(&ux->solar.prod_val.str, energy_prod_solar(&energy, &ux->star));

        ux->burner.show = tech_known(tech, item_burner);
        ui_str_set_u64(&ux->burner.count.str, chunk_count(chunk, item_burner));
        ui_str_set_scaled(&ux->burner.total_val.str, energy.item.burner);

        ux->kwheel.show = tech_known(tech, item_kwheel);
        ui_str_set_u64(&ux->kwheel.count.str, energy.kwheel);
        ui_str_set_scaled(&ux->kwheel.total_val.str, energy_prod_kwheel(&energy, &ux->star));
        energy.kwheel = 1;
        ui_str_set_scaled(&ux->kwheel.prod_val.str, energy_prod_kwheel(&energy, &ux->star));

        ux->battery.show = tech_known(tech, item_battery);
        ui_str_set_u64(&ux->battery.count.str, energy.battery);
        ui_str_set_scaled(&ux->battery.total_val.str, energy_battery_cap(&energy));
        ui_str_set_scaled(&ux->battery.prod_val.str, im_battery_storage_cap);
    }
}

static void ux_star_event(void *state)
{
    struct ux_star *ux = state;

    for (auto ev = ev_select_item(); ev; ev = nullptr) {
        if (!coord_eq(ev->star, ux->id)) ux_star_show(ev->star);
        ui_tree_select(&ux->control_list, ev->item);
        ui_tree_select(&ux->factory_list, ev->item);
    }

    if (ui_button_event(&ux->goto_map)) ux_map_show(ux->id);
    if (ui_button_event(&ux->goto_factory)) ux_factory_show(ux->id, 0);
    if (ui_button_event(&ux->goto_log)) ux_log_show(ux->id);

    if (ui_link_event(&ux->coord_val))
        ui_clipboard_copy_hex(coord_to_u64(ux->star.coord));

    ui_tabs_event(&ux->tabs);
    switch (ui_tabs_selected(&ux->tabs))
    {

    case ux_star_control: {
        if (ui_tree_event(&ux->control_list)) {
            im_id id = ux->control_list.selected;
            if (im_id_seq(id)) ux_item_show(id, ux->id);
        }
        break;
    }

    case ux_star_factory: {
        if (ui_tree_event(&ux->factory_list)) {
            im_id id = ux->factory_list.selected;
            if (im_id_seq(id)) ux_item_show(id, ux->id);
        }
        break;
    }

    case ux_star_logistics: {
        if (ui_button_event(&ux->pills.toggle)) ux_pills_show(ux->id);
        if (ui_button_event(&ux->workers.toggle)) ux_workers_show(ux->id);
        if (ui_button_event(&ux->energy_toggle)) ux_energy_show(ux->id);
        break;
    }

    default: { assert(false); }
    }
}

static void ux_star_render(void *state, struct ui_layout *layout)
{
    struct ux_star *ux = state;

    ui_button_render(&ux->goto_log, layout);
    ui_button_render(&ux->goto_map, layout);
    ui_button_render(&ux->goto_factory, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ux->name, layout);
    ui_label_render(&ux->name_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ux->coord, layout);
    ui_link_render(&ux->coord_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    struct dim cell = engine_cell();
    {
        uint32_t energy = ux->star.energy;
        ui_str_set_scaled(&ux->energy_val.str, energy);
        ux->energy_val.s.fg = rgba_gray(0x11 * u64_log2(energy));

        ui_label_render(&ux->energy, layout);
        ui_label_render(&ux->energy_val, layout);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < items_natural_len; ++i) {
            if (i == items_natural_len-1)
                ui_layout_sep_x(layout, (ux_star_elems_col_len+1)*2 * cell.w);

            ui_str_setc(&ux->elem.str, ux_star_elems[i]);
            ui_label_render(&ux->elem, layout);

            uint16_t value = ux->star.elems[i];
            ui_str_set_scaled(&ux->elem_val.str, value);
            ux->elem_val.s.fg = rgba_gray(0x11 * u64_log2(value));
            ui_label_render(&ux->elem_val, layout);

            size_t col = i % 5;
            if (col < 4) ui_layout_sep_col(layout);
            else ui_layout_next_row(layout);
        }

        ui_layout_next_row(layout);
        ui_layout_sep_row(layout);
    }

    ui_tabs_render(&ux->tabs, layout);
    ui_layout_next_row(layout);

    switch (ui_tabs_selected(&ux->tabs))
    {

    case ux_star_control: {
        ui_tree_render(&ux->control_list, layout);
        break;
    }

    case ux_star_factory: {
        ui_tree_render(&ux->factory_list, layout);
        break;
    }

    case ux_star_logistics: {
        ui_button_render(&ux->pills.toggle, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->pills.count, layout);
        ui_label_render(&ux->pills.count_val, layout);
        ui_layout_next_row(layout);

        ui_layout_sep_row(layout);

        ui_button_render(&ux->workers.toggle, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->workers.count, layout);
        ui_label_render(&ux->workers.count_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->workers.queue, layout);
        ui_label_render(&ux->workers.queue_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->workers.idle, layout);
        ui_label_render(&ux->workers.idle_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->workers.fail, layout);
        ui_label_render(&ux->workers.fail_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->workers.clean, layout);
        ui_label_render(&ux->workers.clean_val, layout);
        ui_layout_next_row(layout);

        ui_layout_sep_row(layout);

        ui_button_render(&ux->energy_toggle, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->need, layout);
        ui_label_render(&ux->need_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->consumed, layout);
        ui_label_render(&ux->consumed_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->produced, layout);
        ui_label_render(&ux->produced_val, layout);
        ui_layout_next_row(layout);
        ui_label_render(&ux->stored, layout);
        ui_label_render(&ux->stored_val, layout);
        ui_layout_next_row(layout);

        if (ux->fusion.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ux->fusion.name, layout);
            ui_label_render(&ux->fusion.count, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->fusion.prod, layout);
            ui_label_render(&ux->fusion.prod_val, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->fusion.total, layout);
            ui_label_render(&ux->fusion.total_val, layout);
            ui_layout_next_row(layout);
        }

        if (ux->solar.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ux->solar.name, layout);
            ui_label_render(&ux->solar.count, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->solar.prod, layout);
            ui_label_render(&ux->solar.prod_val, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->solar.total, layout);
            ui_label_render(&ux->solar.total_val, layout);
            ui_layout_next_row(layout);
        }

        if (ux->burner.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ux->burner.name, layout);
            ui_label_render(&ux->burner.count, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->burner.total, layout);
            ui_label_render(&ux->burner.total_val, layout);
            ui_layout_next_row(layout);
        }

        if (ux->kwheel.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ux->kwheel.name, layout);
            ui_label_render(&ux->kwheel.count, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->kwheel.prod, layout);
            ui_label_render(&ux->kwheel.prod_val, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->kwheel.total, layout);
            ui_label_render(&ux->kwheel.total_val, layout);
            ui_layout_next_row(layout);
        }

        if (ux->battery.show) {
            ui_layout_sep_row(layout);
            ui_label_render(&ux->battery.name, layout);
            ui_label_render(&ux->battery.count, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->battery.prod, layout);
            ui_label_render(&ux->battery.prod_val, layout);
            ui_layout_next_row(layout);
            ui_label_render(&ux->battery.total, layout);
            ui_label_render(&ux->battery.total_val, layout);
            ui_layout_next_row(layout);
        }

        break;
    }

    default: { assert(false); }
    }
}
