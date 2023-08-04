/* ui_tapes.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/tech.h"
#include "items/lab/lab.h"
#include "db/tapes.h"

static void ui_tapes_free(void *);
static void ui_tapes_update(void *);
static bool ui_tapes_event(void *, SDL_Event *);
static void ui_tapes_render(void *, struct ui_layout *, SDL_Renderer *);


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
    const struct font *font = ui_st.font.base;
    int tree_w = (item_str_len + 3) * font->glyph_w;
    int tape_w = (item_str_len + 8 + 1) * font->glyph_w;

    struct ui_tapes *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_tapes) {
        .tree_w = tree_w,
        .tape_w = tape_w,

        .panel = ui_panel_title(
                make_dim(tree_w + font->glyph_w, ui_layout_inf),
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
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
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
    render_push_event(ev_tape_select, tape, 0);
}

static bool ui_tapes_selected(struct ui_tapes *ui)
{
    return ui->tree.selected && ui->tree.selected < items_max;
}

static void ui_tapes_update_cat(
        struct ui_tapes *ui, const char *name, enum item first, enum item last)
{
    const struct tech *tech = proxy_tech();

    ui_node parent = ui_tree_index(&ui->tree);
    ui_str_setc(ui_tree_add(&ui->tree, ui_node_nil, first + items_max), name);

    for (enum item it = first; it < last; ++it) {
        const struct im_config *config = im_config(it);
        if (!config) continue;

        const struct tape_info *info = tapes_info(it);
        if (!info || info->rank >= 14) continue;

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

    if (!ui_tapes_selected(ui)) {
        ui_panel_resize(ui->panel,
                make_dim(
                        ui->tree_w + ui_st.font.base->glyph_w,
                        ui_layout_inf));
        return;
    }

    ui_panel_resize(ui->panel,
            make_dim(
                    ui->tree_w + ui->tape_w + ui_st.font.base->glyph_w,
                    ui_layout_inf));

    enum item item = ui->tree.selected;
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(item);
    assert(tape);

    ui_str_set_item(&ui->name.str, item);

    ui_lab_bits_update(&ui->lab_val, tech, item);
    ui_str_set_scaled(&ui->energy_val.str, tape_energy(tape));

    ui_str_set_item(&ui->host_val.str, tape_host(tape));

    ui_scroll_update(&ui->scroll, tape ? tape_len(tape) : 0);
}

static void ui_tapes_event_help(struct ui_tapes *ui)
{
    enum item item = ui->tree.selected;

    char path[man_path_max] = {0};
    size_t len = snprintf(path, sizeof(path),
            "/items/%s", item_str_c(item));

    struct link link = man_link(path, len);
    if (link_is_nil(link)) {
        render_log(st_error, "unable to open link to '%s'", path);
        return;
    }

    ui_man_show_slot(link, ui_slot_right);
}

static bool ui_tapes_show_help(struct ui_tapes *ui)
{
    return
        item_is_active(ui->tree.selected) ||
        item_is_logistics(ui->tree.selected);
}

static bool ui_tapes_event(void *state, SDL_Event *ev)
{
    struct ui_tapes *ui = state;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_tree_event(&ui->tree, ev))) {
        if (ret == ui_action) ui_tapes_update(ui);
        return true;
    }

    if (!ui_tapes_selected(ui)) return false;

    if ((ret = ui_button_event(&ui->help, ev))) {
        if (ret != ui_action) return true;
        ui_tapes_event_help(ui);
        return true;
    }

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return true;

    if (ui_tapes_show_help(ui)) {
        if ((ret = ui_link_event(&ui->host_val, ev))) {
            if (ret != ui_action) return true;
            ui_tapes_show(tape_host(tapes_get(ui->tree.selected)));
            return true;
        }
    }

    return false;
}

// \TODO: basically a copy of ui_tape. Need to not do that.
static void ui_tapes_render_tape(
        struct ui_tapes *ui,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    const struct font *font = ui_st.font.base;
    const struct tech *tech = proxy_tech();
    const struct tape *tape = tapes_get(ui->tree.selected);

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_x(&inner, font->glyph_w);

        struct tape_ret ret = tape_at(tape, i);

        if (tape_state_item(ret.state) && !tech_learned(tech, ret.item))
            ui_label_render(&ui->known, &inner, renderer);
        else ui_layout_sep_x(&inner, font->glyph_w);

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
        ui_label_render(label, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}

static void ui_tapes_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_tapes *ui = state;

    ui_tree_render(&ui->tree, layout, renderer);

    if (!ui_tapes_selected(ui)) return;

    ui_layout_sep_col(layout);
    struct ui_layout inner = ui_layout_inner(layout);

    if (ui_tapes_show_help(ui)) {
        ui_layout_dir_hori(layout, ui_layout_right_left);
        ui_button_render(&ui->help, layout, renderer);
        ui_layout_dir_hori(layout, ui_layout_left_right);
    }

    ui_label_render(&ui->name, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->lab, &inner, renderer);
    ui_lab_bits_render(&ui->lab_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_label_render(&ui->energy, &inner, renderer);
    ui_label_render(&ui->energy_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->host, &inner, renderer);
    ui_link_render(&ui->host_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_row(&inner);

    ui_label_render(&ui->tape, &inner, renderer);
    ui_layout_next_row(&inner);
    ui_tapes_render_tape(ui, &inner, renderer);
}
