/* ux_tapes.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_tapes_free(void *);
static void ux_tapes_update(void *);
static void ux_tapes_event(void *);
static void ux_tapes_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

struct ux_tapes
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

void ux_tapes_alloc(struct ux_view_state *state)
{
    struct dim cell = engine_cell();
    int tree_w = (item_str_len + 3) * cell.w;
    int tape_w = (item_str_len + 8 + 1) * cell.w;

    struct ux_tapes *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_tapes) {
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

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_tapes,
        .slots = ux_slot_left,
        .panel = ux->panel,
        .fn = {
            .free = ux_tapes_free,
            .update_frame = ux_tapes_update,
            .event = ux_tapes_event,
            .render = ux_tapes_render,
        },
    };
}

static void ux_tapes_free(void *state)
{
    struct ux_tapes *ux = state;

    ui_panel_free(ux->panel);

    ui_tree_free(&ux->tree);

    ui_label_free(&ux->name);
    ui_button_free(&ux->help);

    ui_label_free(&ux->lab);

    ui_label_free(&ux->energy);
    ui_label_free(&ux->energy_val);

    ui_label_free(&ux->host);
    ui_link_free(&ux->host_val);

    ui_label_free(&ux->tape);
    ui_scroll_free(&ux->scroll);
    ui_label_free(&ux->index);
    ui_label_free(&ux->in);
    ui_label_free(&ux->work);
    ui_label_free(&ux->out);

    free(ux);
}

void ux_tapes_show(enum item tape)
{
    struct ux_tapes *ux = ux_state(ux_view_tapes);

    if (tape) ui_tree_select(&ux->tree, tape);
    else ui_tree_clear(&ux->tree);

    if (!tape) { ux_hide(ux_view_tapes); return; }

    ux_tapes_update(ux);
    ux_show(ux_view_tapes);
}

static bool ux_tapes_selected(struct ux_tapes *ux)
{
    return ux->tree.selected && ux->tree.selected < items_max;
}

static void ux_tapes_update_cat(
        struct ux_tapes *ux, const char *name, enum item first, enum item last)
{
    const struct tech *tech = proxy_tech();

    ui_tree_node parent = ui_tree_index(&ux->tree);
    ui_str_setc(ui_tree_add(&ux->tree, ui_tree_node_nil, first + items_max), name);

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

        ui_str_set_item(ui_tree_add(&ux->tree, parent, it), it);
    }
}

static void ux_tapes_update(void *state)
{
    struct ux_tapes *ux = state;

    ui_tree_reset(&ux->tree);
    ux_tapes_update_cat(ux, "Natural", items_natural_first, items_natural_last);
    ux_tapes_update_cat(ux, "Synthesized", items_synth_first, items_synth_last);
    ux_tapes_update_cat(ux, "Passive", items_passive_first, items_passive_last);
    ux_tapes_update_cat(ux, "Active", items_active_first, items_active_last);
    ux_tapes_update_cat(ux, "Logistics", items_logistics_first, items_logistics_last);

    struct dim cell = engine_cell();

    if (!ux_tapes_selected(ux)) {
        ui_panel_resize(ux->panel, make_dim(ux->tree_w + cell.w, ui_layout_inf));
        return;
    }

    ui_panel_resize(ux->panel, make_dim(ux->tree_w + ux->tape_w + cell.w, ui_layout_inf));

    enum item item = ux->tree.selected;
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(item);
    assert(tape);

    ui_str_set_item(&ux->name.str, item);

    ui_lab_bits_update(&ux->lab_val, tech, item);
    ui_str_set_scaled(&ux->energy_val.str, tape_energy(tape));

    ui_str_set_item(&ux->host_val.str, tape_host(tape));

    ui_scroll_update_rows(&ux->scroll, tape ? tape_len(tape) : 0);
}

static bool ux_tapes_show_help(struct ux_tapes *ux)
{
    return
        item_is_active(ux->tree.selected) ||
        item_is_logistics(ux->tree.selected);
}

static void ux_tapes_event(void *state)
{
    struct ux_tapes *ux = state;

    if (ui_tree_event(&ux->tree)) ux_tapes_update(ux);

    if (!ux_tapes_selected(ux)) return;

    if (ui_button_event(&ux->help))
        ux_man_show_slot_path(ux_slot_right,
                "/items/%s", item_str_c(ux->tree.selected));

    ui_scroll_event(&ux->scroll);

    if (ux_tapes_show_help(ux)) {
        if (ui_link_event(&ux->host_val))
            ux_tapes_show(tape_host(tapes_get(ux->tree.selected)));
    }
}

// \TODO: basically a copy of ui_tape. Need to not do that.
static void ux_tapes_render_tape(struct ux_tapes *ux, struct ui_layout *layout)
{
    struct ui_layout inner = ui_scroll_render(&ux->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;

    struct dim cell = engine_cell();
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(ux->tree.selected);

    size_t first = ui_scroll_first_row(&ux->scroll);
    size_t last = ui_scroll_last_row(&ux->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ux->index.str, i);
        ui_label_render(&ux->index, &inner);
        ui_layout_sep_x(&inner, cell.w);

        struct tape_ret ret = tape_at(tape, i);

        if (tape_state_item(ret.state) && !tech_learned(tech, ret.item))
            ui_label_render(&ux->known, &inner);
        else ui_layout_sep_x(&inner, cell.w);

        struct ui_label *label = NULL;
        switch (ret.state)
        {
        case tape_input: { label = &ux->in; break; }
        case tape_work: { label = &ux->work; break; }
        case tape_output: { label = &ux->out; break; }
        case tape_eof:
        default: { assert(false); }
        }

        if (tape_state_item(ret.state))
            ui_str_set_item(&label->str, ret.item);
        ui_label_render(label, &inner);

        ui_layout_next_row(&inner);
    }
}

static void ux_tapes_render(void *state, struct ui_layout *layout)
{
    struct ux_tapes *ux = state;

    ui_tree_render(&ux->tree, layout);

    if (!ux_tapes_selected(ux)) return;

    ui_layout_sep_col(layout);
    struct ui_layout inner = ui_layout_inner(layout);

    if (ux_tapes_show_help(ux)) {
        ui_layout_dir_hori(layout, ui_layout_right_left);
        ui_button_render(&ux->help, layout);
        ui_layout_dir_hori(layout, ui_layout_left_right);
    }

    ui_label_render(&ux->name, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ux->lab, &inner);
    ui_lab_bits_render(&ux->lab_val, &inner);
    ui_layout_next_row(&inner);

    ui_label_render(&ux->energy, &inner);
    ui_label_render(&ux->energy_val, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ux->host, &inner);
    ui_link_render(&ux->host_val, &inner);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ux->tape, &inner);
    ui_layout_next_row(&inner);
    ux_tapes_render_tape(ux, &inner);
}
