/* ui_tapes.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/tech.h"
#include "items/lab/lab.h"
#include "db/tapes.h"

static void ui_tapes_free(void *);
static void ui_tapes_update(void *);
static void ui_tapes_event(void *);
static void ui_tapes_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

struct ui_tapes
{
    int tree_w;
    int tape_w;

    struct ui_panel *panel;

    struct ui_tree tree;

    struct ui_label name;
    struct ui_button help;

    struct ui_label lab;
    struct ui_lab_bits lab_val;

    struct ui_label energy, energy_val;

    struct ui_label host;
    struct ui_link host_val;

    struct ui_label tape;
    struct ui_scroll scroll;
    struct ui_label index, known, in, work, out;
};

void ui_tapes_alloc(struct ui_view_state *state)
{
    struct dim cell = engine_cell();
    int tree_w = (item_str_len + 3) * cell.w;
    int tape_w = (item_str_len + 8 + 1) * cell.w;

    struct ui_tapes *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_tapes) {
        .tree_w = tree_w,
        .tape_w = tape_w,

        .panel = ui_panel_title(
                make_dim(tree_w + cell.w, ui_layout_inf),
                ui_str_c("tapes")),

        .tree = ui_tree_new(make_dim(tree_w, ui_layout_inf), symbol_cap),

        .name = ui_label_new(ui_str_v(item_str_len)),
        .help = ui_button_new_s(&ui_st.button.line, ui_str_c("?")),

        .lab = ui_label_new(ui_str_c("lab:  ")),
        .lab_val = ui_lab_bits_new(),

        .energy = ui_label_new(ui_str_c("energy: ")),
        .energy_val = ui_label_new(ui_str_v(str_scaled_len)),

        .host = ui_label_new(ui_str_c("host: ")),
        .host_val = ui_link_new(ui_str_v(item_str_len)),

        .tape = ui_label_new(ui_str_c("tape: ")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), cell),
        .index = ui_label_new_s(&ui_st.label.index, ui_str_v(2)),
        .in = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),
        .work = ui_label_new_s(&ui_st.label.work, ui_str_c("work")),
        .out = ui_label_new_s(&ui_st.label.out, ui_str_v(item_str_len)),
        .known = ui_label_new(ui_str_c("*")),
    };

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_tapes,
        .slots = ui_slot_left,
        .panel = ui->panel,
        .fn = {
            .free = ui_tapes_free,
            .update_frame = ui_tapes_update,
            .event = ui_tapes_event,
            .render = ui_tapes_render,
        },
    };
}

static void ui_tapes_free(void *state)
{
    struct ui_tapes *ui = state;

    ui_panel_free(ui->panel);

    ui_tree_free(&ui->tree);

    ui_label_free(&ui->name);
    ui_button_free(&ui->help);

    ui_label_free(&ui->lab);

    ui_label_free(&ui->energy);
    ui_label_free(&ui->energy_val);

    ui_label_free(&ui->host);
    ui_link_free(&ui->host_val);

    ui_label_free(&ui->tape);
    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->in);
    ui_label_free(&ui->work);
    ui_label_free(&ui->out);

    free(ui);
}

void ui_tapes_show(enum item tape)
{
    struct ui_tapes *ui = ui_state(ui_view_tapes);

    if (tape) ui_tree_select(&ui->tree, tape);
    else ui_tree_clear(&ui->tree);

    if (!tape) { ui_hide(ui_view_tapes); return; }

    ui_tapes_update(ui);
    ui_show(ui_view_tapes);
}

static bool ui_tapes_selected(struct ui_tapes *ui)
{
    return ui->tree.selected && ui->tree.selected < items_max;
}

static void ui_tapes_update_cat(
        struct ui_tapes *ui, const char *name, enum item first, enum item last)
{
    const struct tech *tech = proxy_tech();

    ui_tree_node parent = ui_tree_index(&ui->tree);
    ui_str_setc(ui_tree_add(&ui->tree, ui_tree_node_nil, first + items_max), name);

    for (enum item it = first; it < last; ++it) {
        const struct im_config *config = im_config(it);
        if (!config) continue;

        const struct tape_info *info = tapes_info(it);
        if (!info || info->rank >= im_rank_sys) continue;

        const struct tape *tape = tapes_get(it);
        if (!tape) continue;

        if (!tech_known(tech, tape_host(tape))) continue;

        if (!tech_known(tech, it)) {
            struct tape_set inputs = info->inputs;
            struct tape_set known = tech_known_list(tech);
            if (tape_set_intersect(&known, &inputs) != tape_set_len(&inputs))
                continue;
        }

        ui_str_set_item(ui_tree_add(&ui->tree, parent, it), it);
    }
}

static void ui_tapes_update(void *state)
{
    struct ui_tapes *ui = state;

    ui_tree_reset(&ui->tree);
    ui_tapes_update_cat(ui, "Natural", items_natural_first, items_natural_last);
    ui_tapes_update_cat(ui, "Synthesized", items_synth_first, items_synth_last);
    ui_tapes_update_cat(ui, "Passive", items_passive_first, items_passive_last);
    ui_tapes_update_cat(ui, "Active", items_active_first, items_active_last);
    ui_tapes_update_cat(ui, "Logistics", items_logistics_first, items_logistics_last);

    struct dim cell = engine_cell();

    if (!ui_tapes_selected(ui)) {
        ui_panel_resize(ui->panel, make_dim(ui->tree_w + cell.w, ui_layout_inf));
        return;
    }

    ui_panel_resize(ui->panel, make_dim(ui->tree_w + ui->tape_w + cell.w, ui_layout_inf));

    enum item item = ui->tree.selected;
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(item);
    assert(tape);

    ui_str_set_item(&ui->name.str, item);

    ui_lab_bits_update(&ui->lab_val, tech, item);
    ui_str_set_scaled(&ui->energy_val.str, tape_energy(tape));

    ui_str_set_item(&ui->host_val.str, tape_host(tape));

    ui_scroll_update_rows(&ui->scroll, tape ? tape_len(tape) : 0);
}

static bool ui_tapes_show_help(struct ui_tapes *ui)
{
    return
        item_is_active(ui->tree.selected) ||
        item_is_logistics(ui->tree.selected);
}

static void ui_tapes_event(void *state)
{
    struct ui_tapes *ui = state;

    if (ui_tree_event(&ui->tree)) ui_tapes_update(ui);

    if (!ui_tapes_selected(ui)) return;

    if (ui_button_event(&ui->help))
        ui_man_show_slot_path(ui_slot_right,
                "/items/%s", item_str_c(ui->tree.selected));

    ui_scroll_event(&ui->scroll);

    if (ui_tapes_show_help(ui)) {
        if (ui_link_event(&ui->host_val))
            ui_tapes_show(tape_host(tapes_get(ui->tree.selected)));
    }
}

// \TODO: basically a copy of ui_tape. Need to not do that.
static void ui_tapes_render_tape(struct ui_tapes *ui, struct ui_layout *layout)
{
    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;

    struct dim cell = engine_cell();
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(ui->tree.selected);

    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner);
        ui_layout_sep_x(&inner, cell.w);

        struct tape_ret ret = tape_at(tape, i);

        if (tape_state_item(ret.state) && !tech_learned(tech, ret.item))
            ui_label_render(&ui->known, &inner);
        else ui_layout_sep_x(&inner, cell.w);

        struct ui_label *label = NULL;
        switch (ret.state)
        {
        case tape_input: { label = &ui->in; break; }
        case tape_work: { label = &ui->work; break; }
        case tape_output: { label = &ui->out; break; }
        case tape_eof:
        default: { assert(false); }
        }

        if (tape_state_item(ret.state))
            ui_str_set_item(&label->str, ret.item);
        ui_label_render(label, &inner);

        ui_layout_next_row(&inner);
    }
}

static void ui_tapes_render(void *state, struct ui_layout *layout)
{
    struct ui_tapes *ui = state;

    ui_tree_render(&ui->tree, layout);

    if (!ui_tapes_selected(ui)) return;

    ui_layout_sep_col(layout);
    struct ui_layout inner = ui_layout_inner(layout);

    if (ui_tapes_show_help(ui)) {
        ui_layout_dir_hori(layout, ui_layout_right_left);
        ui_button_render(&ui->help, layout);
        ui_layout_dir_hori(layout, ui_layout_left_right);
    }

    ui_label_render(&ui->name, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->lab, &inner);
    ui_lab_bits_render(&ui->lab_val, &inner);
    ui_layout_next_row(&inner);

    ui_label_render(&ui->energy, &inner);
    ui_label_render(&ui->energy_val, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->host, &inner);
    ui_link_render(&ui->host_val, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->tape, &inner);
    ui_layout_next_row(&inner);
    ui_tapes_render_tape(ui, &inner);
}
