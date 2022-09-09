/* ui_star.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/energy.h"
#include "items/item.h"
#include "utils/vec.h"


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ui_star_workers
{
    struct ui_label workers, workers_val;
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

    struct ui_panel panel;

    struct ui_button goto_factory, goto_map, goto_log;
    struct ui_label name, name_val;
    struct ui_label coord;
    struct ui_link coord_val;

    struct ui_label energy, energy_val;
    struct ui_label elem, elem_val;

    struct ui_button control, factory, logistic;

    struct ui_tree control_list, factory_list;

    struct ui_label pills, pills_val;

    struct ui_star_workers workers;

    struct ui_label need, need_val;
    struct ui_label consumed, consumed_val;
    struct ui_label produced, produced_val;
    struct ui_label stored, stored_val;
    struct ui_star_energy solar;
    struct ui_star_energy kwheel;
    struct ui_star_energy store;

};

static struct font *ui_star_font(void) { return font_mono6; }

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

struct ui_star *ui_star_new(void)
{
    struct font *font = ui_star_font();
    size_t width = 38 * font->glyph_w;
    struct pos pos = make_pos(render.rect.w - width, ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_star *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_star) {
        .panel = ui_panel_title(pos, dim, ui_str_c("star")),

        .goto_map = ui_button_new(font, ui_str_c("<< map")),
        .goto_factory = ui_button_new(font, ui_str_c("<< factory")),
        .goto_log = ui_button_new(font, ui_str_c("<< log")),

        .name = ui_label_new(font, ui_str_c("name:  ")),
        .name_val = ui_label_new(font, ui_str_v(symbol_cap)),

        .coord = ui_label_new(font, ui_str_c("coord: ")),
        .coord_val = ui_link_new(font, ui_str_v(coord_str_len)),

        .energy = ui_label_new(font, ui_str_c("energy: ")),
        .energy_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .elem = ui_label_new(font, ui_str_c(ui_star_elems[0])),
        .elem_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .control = ui_button_new(font, ui_str_c("control")),
        .factory = ui_button_new(font, ui_str_c("factory")),
        .logistic = ui_button_new(font, ui_str_c("logistic")),

        .control_list = ui_tree_new(
                make_dim(ui_layout_inf, ui_layout_inf), font, im_id_str_len),
        .factory_list = ui_tree_new(
                make_dim(ui_layout_inf, ui_layout_inf), font, im_id_str_len),

        .pills = ui_label_new(font, ui_str_c("pills: ")),
        .pills_val = ui_label_new(font, ui_str_v(10)),

        .workers = (struct ui_star_workers) {
            .workers = ui_label_new(font, ui_str_c("workers: ")),
            .workers_val = ui_label_new(font, ui_str_v(10)),
            .queue = ui_label_new(font, ui_str_c("- queue: ")),
            .queue_val = ui_label_new(font, ui_str_v(10)),
            .idle = ui_label_new(font, ui_str_c("- idle:  ")),
            .idle_val = ui_label_new(font, ui_str_v(10)),
            .fail = ui_label_new(font, ui_str_c("- fail:  ")),
            .fail_val = ui_label_new(font, ui_str_v(10)),
            .clean = ui_label_new(font, ui_str_c("- clean: ")),
            .clean_val = ui_label_new(font, ui_str_v(10)),
        },

        .need = ui_label_new(font, ui_str_c("- need:     ")),
        .need_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        .consumed = ui_label_new(font, ui_str_c("- consumed: ")),
        .consumed_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        .produced = ui_label_new(font, ui_str_c("- produced: ")),
        .produced_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        .stored = ui_label_new(font, ui_str_c("- stored:   ")),
        .stored_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .solar = (struct ui_star_energy) {
            .name = ui_label_new(font, ui_str_c("solar:        ")),
            .count = ui_label_new(font, ui_str_v(10)),
            .prod = ui_label_new(font, ui_str_c("- production: ")),
            .prod_val = ui_label_new(font, ui_str_v(str_scaled_len)),
            .total = ui_label_new(font, ui_str_c("- total:      ")),
            .total_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        },

        .kwheel = (struct ui_star_energy) {
            .name = ui_label_new(font, ui_str_c("k-wheel:      ")),
            .count = ui_label_new(font, ui_str_v(10)),
            .prod = ui_label_new(font, ui_str_c("- production: ")),
            .prod_val = ui_label_new(font, ui_str_v(str_scaled_len)),
            .total = ui_label_new(font, ui_str_c("- total:      ")),
            .total_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        },

        .store = (struct ui_star_energy) {
            .name = ui_label_new(font, ui_str_c("store:      ")),
            .count = ui_label_new(font, ui_str_v(10)),
            .prod = ui_label_new(font, ui_str_c("- capacity: ")),
            .prod_val = ui_label_new(font, ui_str_v(str_scaled_len)),
            .total = ui_label_new(font, ui_str_c("- total:    ")),
            .total_val = ui_label_new(font, ui_str_v(str_scaled_len)),
        },
    };

    ui_panel_hide(&ui->panel);
    ui->control.disabled = true;

    size_t goto_width = (width - font->glyph_w) / 3;
    ui->goto_map.w.dim.w = goto_width;
    ui->goto_factory.w.dim.w = goto_width;
    ui->goto_log.w.dim.w = goto_width;

    return ui;
}

void ui_star_free(struct ui_star *ui) {
    ui_panel_free(&ui->panel);

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

    ui_button_free(&ui->control);
    ui_button_free(&ui->factory);
    ui_button_free(&ui->logistic);

    ui_tree_free(&ui->control_list);
    ui_tree_free(&ui->factory_list);

    ui_label_free(&ui->workers.workers);
    ui_label_free(&ui->workers.workers_val);
    ui_label_free(&ui->workers.queue);
    ui_label_free(&ui->workers.queue_val);
    ui_label_free(&ui->workers.idle);
    ui_label_free(&ui->workers.idle_val);
    ui_label_free(&ui->workers.fail);
    ui_label_free(&ui->workers.fail_val);
    ui_label_free(&ui->workers.clean);
    ui_label_free(&ui->workers.clean_val);

    ui_label_free(&ui->need);
    ui_label_free(&ui->need_val);
    ui_label_free(&ui->consumed);
    ui_label_free(&ui->consumed_val);
    ui_label_free(&ui->produced);
    ui_label_free(&ui->produced_val);
    ui_label_free(&ui->stored);
    ui_label_free(&ui->stored_val);

    ui_label_free(&ui->solar.name);
    ui_label_free(&ui->solar.count);
    ui_label_free(&ui->solar.prod);
    ui_label_free(&ui->solar.prod_val);
    ui_label_free(&ui->solar.total);
    ui_label_free(&ui->solar.total_val);

    ui_label_free(&ui->kwheel.name);
    ui_label_free(&ui->kwheel.count);
    ui_label_free(&ui->kwheel.prod);
    ui_label_free(&ui->kwheel.prod_val);
    ui_label_free(&ui->kwheel.total);
    ui_label_free(&ui->kwheel.total_val);

    ui_label_free(&ui->store.name);
    ui_label_free(&ui->store.count);
    ui_label_free(&ui->store.prod);
    ui_label_free(&ui->store.prod_val);
    ui_label_free(&ui->store.total);
    ui_label_free(&ui->store.total_val);

    free(ui);
}


int16_t ui_star_width(const struct ui_star *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_star_update_list(
        struct chunk *chunk, struct ui_tree *tree, im_list filter)
{
    ui_tree_reset(tree);

    enum item item = ITEM_NIL;
    ui_node parent = ui_node_nil;
    struct vec64 *ids = chunk_list_filter(chunk, filter);

    for (size_t i = 0; i < ids->len; ++i) {
        im_id id = ids->vals[i];

        if (item != im_id_item(id)) {
            item = im_id_item(id);
            parent = ui_tree_index(tree);
            ui_str_set_item(ui_tree_add(tree, ui_node_nil, make_im_id(item, 0)), item);
        }

        ui_str_set_id(ui_tree_add(tree, parent, id), id);
    }

    vec64_free(ids);
}

static void ui_star_update(struct ui_star *ui)
{
    ui_str_set_coord(&ui->coord_val.str, ui->id);

    struct chunk *chunk = proxy_chunk(render.proxy, ui->id);
    if (!chunk) {
        const struct star *star = proxy_star_at(render.proxy, ui->id);
        assert(star);
        ui->star = *star;

        {
            world_seed seed = proxy_seed(render.proxy);
            struct atoms *atoms = proxy_atoms(render.proxy);

            struct symbol sym = {0};
            vm_word name = gen_name_star(ui->id, seed, atoms);
            bool ok = atoms_str(atoms, name, &sym);
            ui_str_set_symbol(&ui->name_val.str, &sym);
            assert(ok);
        }

        ui_tree_clear(&ui->control_list);
        ui_tree_reset(&ui->control_list);
        ui_tree_clear(&ui->factory_list);
        ui_tree_reset(&ui->factory_list);

        ui_str_set_u64(&ui->pills_val.str, 0);

        ui_str_set_u64(&ui->workers.workers_val.str, 0);
        ui_str_set_u64(&ui->workers.idle_val.str, 0);
        ui_str_set_u64(&ui->workers.fail_val.str, 0);
        ui_str_set_u64(&ui->workers.queue_val.str, 0);

        ui_str_set_scaled(&ui->need_val.str, 0);
        ui_str_set_scaled(&ui->consumed_val.str, 0);
        ui_str_set_scaled(&ui->produced_val.str, 0);
        ui_str_set_scaled(&ui->stored_val.str, 0);

        ui->solar.show = false;
        ui->kwheel.show = false;
        ui->store.show = false;
        return;
    }

    {
        struct symbol sym = {0};
        vm_word name = chunk_name(chunk);
        if (atoms_str(proxy_atoms(render.proxy), name, &sym))
            ui_str_set_symbol(&ui->name_val.str, &sym);
        else ui_str_set_hex(&ui->name_val.str, name);
    }

    ui->star = *chunk_star(chunk);
    ui_star_update_list(chunk, &ui->control_list, im_list_control);
    ui_star_update_list(chunk, &ui->factory_list, im_list_factory);

    ui_str_set_u64(&ui->pills_val.str, chunk_scan(chunk, ITEM_PILL));

    {
        struct workers workers = chunk_workers(chunk);
        ui_str_set_u64(&ui->workers.workers_val.str, workers.count);
        ui_str_set_u64(&ui->workers.queue_val.str, workers.queue);
        ui_str_set_u64(&ui->workers.idle_val.str, workers.idle);
        ui_str_set_u64(&ui->workers.fail_val.str, workers.fail);
        ui_str_set_u64(&ui->workers.clean_val.str, workers.clean);
    }

    {
        struct energy energy = *chunk_energy(chunk);
        ui_str_set_scaled(&ui->need_val.str, energy.need);
        ui_str_set_scaled(&ui->consumed_val.str, energy.consumed);
        ui_str_set_scaled(&ui->produced_val.str, energy.produced);
        ui_str_set_scaled(&ui->stored_val.str, energy.current);

        ui->solar.show = energy.solar;
        ui_str_set_u64(&ui->solar.count.str, energy.solar);
        ui_str_set_scaled(&ui->solar.total_val.str, energy_prod_solar(&energy, &ui->star));
        energy.solar = 1;
        ui_str_set_scaled(&ui->solar.prod_val.str, energy_prod_solar(&energy, &ui->star));

        ui->kwheel.show = energy.kwheel;
        ui_str_set_u64(&ui->kwheel.count.str, energy.kwheel);
        ui_str_set_scaled(&ui->kwheel.total_val.str, energy_prod_kwheel(&energy, &ui->star));
        energy.kwheel = 1;
        ui_str_set_scaled(&ui->kwheel.prod_val.str, energy_prod_kwheel(&energy, &ui->star));

        ui->store.show = energy.battery;
        ui_str_set_u64(&ui->store.count.str, energy.battery);
        ui_str_set_scaled(&ui->store.total_val.str, energy_battery(&energy));
        energy.battery = 1;
        ui_str_set_scaled(&ui->store.prod_val.str, energy_battery(&energy));
    }
}

static bool ui_star_event_user(struct ui_star *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        ui->id = coord_nil();
        ui_tree_clear(&ui->control_list);
        ui_tree_clear(&ui->factory_list);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_star_update(ui);
        return false;
    }

    case EV_STAR_SELECT: {
        ui->id = coord_from_u64((uintptr_t) ev->user.data1);
        ui_star_update(ui);
        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_STAR_CLEAR: {
        ui->id = coord_nil();
        ui_panel_hide(&ui->panel);
        ui_tree_clear(&ui->control_list);
        ui_tree_clear(&ui->factory_list);
        return false;
    }

    case EV_ITEM_SELECT: {
        struct coord coord = coord_from_u64((uintptr_t) ev->user.data2);
        if (!coord_eq(coord, ui->id)) {
            ui->id = coord;
            ui_star_update(ui);
        }

        im_id selected = (uintptr_t) ev->user.data1;
        ui_tree_select(&ui->control_list, selected);
        ui_tree_select(&ui->factory_list, selected);

        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_ITEM_CLEAR: {
        ui_tree_clear(&ui->control_list);
        ui_tree_clear(&ui->factory_list);
        return false;
    }

    default: { return false; }
    }
}


bool ui_star_event(struct ui_star *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_star_event_user(ui, ev)) return false;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) {
        if (ret == ui_consume) render_push_event(EV_STAR_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if ((ret = ui_button_event(&ui->goto_map, ev))) {
        render_push_event(EV_MAP_GOTO, coord_to_u64(ui->id), 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->goto_factory, ev))) {
        render_push_event(EV_FACTORY_SELECT, coord_to_u64(ui->id), 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->goto_log, ev))) {
        render_push_event(EV_LOG_SELECT, coord_to_u64(ui->id), 0);
        return true;
    }

    if ((ret = ui_link_event(&ui->coord_val, ev))) {
        ui_clipboard_copy_hex(&render.ui.board, coord_to_u64(ui->star.coord));
        return true;
    }

    if ((ret = ui_button_event(&ui->control, ev))) {
        ui->control.disabled = true;
        ui->factory.disabled = ui->logistic.disabled = false;
        return true;
    }

    if ((ret = ui_button_event(&ui->factory, ev))) {
        ui->factory.disabled = true;
        ui->control.disabled = ui->logistic.disabled = false;
        return true;
    }

    if ((ret = ui_button_event(&ui->logistic, ev))) {
        ui->logistic.disabled = true;
        ui->control.disabled = ui->factory.disabled = false;
        return true;
    }

    if (ui->control.disabled && (ret = ui_tree_event(&ui->control_list, ev))) {
        im_id id = ui->control_list.selected;
        if (ret == ui_action && im_id_seq(id))
            render_push_event(EV_ITEM_SELECT, id, coord_to_u64(ui->star.coord));
        return true;
    }

    if (ui->factory.disabled && (ret = ui_tree_event(&ui->factory_list, ev))) {
        im_id id = ui->factory_list.selected;
        if (ret == ui_action && im_id_seq(id))
            render_push_event(EV_ITEM_SELECT, id, coord_to_u64(ui->star.coord));
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_star_render(struct ui_star *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    struct font *font = ui_star_font();

    ui_button_render(&ui->goto_log, &layout, renderer);
    ui_button_render(&ui->goto_map, &layout, renderer);
    ui_button_render(&ui->goto_factory, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_y(&layout, font->glyph_h);

    ui_label_render(&ui->name, &layout, renderer);
    ui_label_render(&ui->name_val, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_label_render(&ui->coord, &layout, renderer);
    ui_link_render(&ui->coord_val, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_y(&layout, font->glyph_h);

    {
        uint32_t energy = ui->star.energy;
        ui_str_set_scaled(&ui->energy_val.str, energy);
        ui->energy_val.fg = rgba_gray(0x11 * u64_log2(energy));

        ui_label_render(&ui->energy, &layout, renderer);
        ui_label_render(&ui->energy_val, &layout, renderer);
        ui_layout_next_row(&layout);

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) {
            if (i == ITEMS_NATURAL_LEN-1)
                ui_layout_sep_x(&layout, (ui_star_elems_col_len+1)*2 * font->glyph_w);

            ui_str_setc(&ui->elem.str, ui_star_elems[i]);
            ui_label_render(&ui->elem, &layout, renderer);

            uint16_t value = ui->star.elems[i];
            ui_str_set_scaled(&ui->elem_val.str, value);
            ui->elem_val.fg = rgba_gray(0x11 * u64_log2(value));
            ui_label_render(&ui->elem_val, &layout, renderer);

            size_t col = i % 5;
            if (col < 4) ui_layout_sep_x(&layout, font->glyph_w);
            else ui_layout_next_row(&layout);
        }

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, font->glyph_h);
    }

    ui_button_render(&ui->control, &layout, renderer);
    ui_button_render(&ui->factory, &layout, renderer);
    ui_button_render(&ui->logistic, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    if (ui->control.disabled)
        ui_tree_render(&ui->control_list, &layout, renderer);

    if (ui->factory.disabled)
        ui_tree_render(&ui->factory_list, &layout, renderer);

    if (ui->logistic.disabled) {
        ui_label_render(&ui->pills, &layout, renderer);
        ui_label_render(&ui->pills_val, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_layout_sep_y(&layout, font->glyph_h);

        ui_label_render(&ui->workers.workers, &layout, renderer);
        ui_label_render(&ui->workers.workers_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->workers.queue, &layout, renderer);
        ui_label_render(&ui->workers.queue_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->workers.idle, &layout, renderer);
        ui_label_render(&ui->workers.idle_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->workers.fail, &layout, renderer);
        ui_label_render(&ui->workers.fail_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->workers.clean, &layout, renderer);
        ui_label_render(&ui->workers.clean_val, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_layout_sep_y(&layout, font->glyph_h);

        ui_label_render(&ui->energy, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->need, &layout, renderer);
        ui_label_render(&ui->need_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->consumed, &layout, renderer);
        ui_label_render(&ui->consumed_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->produced, &layout, renderer);
        ui_label_render(&ui->produced_val, &layout, renderer);
        ui_layout_next_row(&layout);
        ui_label_render(&ui->stored, &layout, renderer);
        ui_label_render(&ui->stored_val, &layout, renderer);
        ui_layout_next_row(&layout);

        if (ui->solar.show) {
            ui_layout_sep_y(&layout, font->glyph_h);
            ui_label_render(&ui->solar.name, &layout, renderer);
            ui_label_render(&ui->solar.count, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->solar.prod, &layout, renderer);
            ui_label_render(&ui->solar.prod_val, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->solar.total, &layout, renderer);
            ui_label_render(&ui->solar.total_val, &layout, renderer);
            ui_layout_next_row(&layout);
        }

        if (ui->kwheel.show) {
            ui_layout_sep_y(&layout, font->glyph_h);
            ui_label_render(&ui->kwheel.name, &layout, renderer);
            ui_label_render(&ui->kwheel.count, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->kwheel.prod, &layout, renderer);
            ui_label_render(&ui->kwheel.prod_val, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->kwheel.total, &layout, renderer);
            ui_label_render(&ui->kwheel.total_val, &layout, renderer);
            ui_layout_next_row(&layout);
        }

        if (ui->store.show) {
            ui_layout_sep_y(&layout, font->glyph_h);
            ui_label_render(&ui->store.name, &layout, renderer);
            ui_label_render(&ui->store.count, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->store.prod, &layout, renderer);
            ui_label_render(&ui->store.prod_val, &layout, renderer);
            ui_layout_next_row(&layout);
            ui_label_render(&ui->store.total, &layout, renderer);
            ui_label_render(&ui->store.total_val, &layout, renderer);
            ui_layout_next_row(&layout);
        }
    }
}
