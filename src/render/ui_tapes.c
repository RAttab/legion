/* ui_tapes.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/tech.h"
#include "items/lab/lab.h"


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

struct ui_tapes
{
    int height;
    int tree_w;
    int tape_w;

    struct ui_panel panel;

    struct ui_tree tree;

    struct ui_label name;
    struct ui_button help;

    struct ui_label lab;
    struct im_lab_bits lab_val;

    struct ui_label energy, energy_val;

    struct ui_label host;
    struct ui_link host_val;

    struct ui_label tape;
    struct ui_scroll scroll;
    struct ui_label index, known, in, work, out;
};

static struct font *ui_tapes_font(void) { return font_mono6; }

struct ui_tapes *ui_tapes_new(void)
{
    struct font *font = ui_tapes_font();
    struct pos pos = make_pos(0, ui_topbar_height());

    int height = render.rect.h - pos.y - ui_status_height();
    int tree_w = (item_str_len + 3) * font->glyph_w;
    int tape_w = (item_str_len + 8 + 1) * font->glyph_w;
    struct dim dim = make_dim(tree_w + font->glyph_w, height);

    struct ui_tapes *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_tapes) {
        .height = height,
        .tree_w = tree_w,
        .tape_w = tape_w,

        .panel = ui_panel_title(pos, dim, ui_str_c("tapes")),

        .tree = ui_tree_new(make_dim(tree_w, ui_layout_inf), font, symbol_cap),

        .name = ui_label_new(ui_str_v(item_str_len)),
        .help = ui_button_new_pad(font, ui_str_c("?"), make_dim(6, 0)),

        .lab = ui_label_new(ui_str_c("lab:  ")),
        .lab_val = im_lab_bits_new(font),

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

    ui_panel_hide(&ui->panel);
    return ui;
}


void ui_tapes_free(struct ui_tapes *ui) {
    ui_panel_free(&ui->panel);

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

static bool ui_tapes_selected(struct ui_tapes *ui)
{
    return ui->tree.selected && ui->tree.selected < ITEM_MAX;
}

static void ui_tapes_update_cat(
        struct ui_tapes *ui, const char *name, enum item first, enum item last)
{
    const struct tech *tech = proxy_tech(render.proxy);

    ui_node parent = ui_tree_index(&ui->tree);
    ui_str_setc(ui_tree_add(&ui->tree, ui_node_nil, first + ITEM_MAX), name);

    for (enum item it = first; it < last; ++it) {
        const struct im_config *config = im_config(it);
        if (!config) continue;

        const struct tape_info *info = tapes_info(it);
        if (!info) continue;

        const struct tape *tape = tapes_get(it);
        if (!tape) continue;

        if (!tech_known(tech, tape_host(tape))) continue;

        if (!tech_known(tech, it)) {
            struct tape_set reqs = info->reqs;
            struct tape_set known = tech_known_list(tech);
            if (tape_set_intersect(&known, &reqs) != tape_set_len(&reqs))
                continue;
        }

        ui_str_set_item(ui_tree_add(&ui->tree, parent, it), it);
    }
}

static void ui_tapes_update(struct ui_tapes *ui, enum item select)
{
    struct font *font = ui_tapes_font();

    ui_tree_reset(&ui->tree);
    ui_tapes_update_cat(ui, "Natural", ITEM_NATURAL_FIRST, ITEM_NATURAL_LAST);
    ui_tapes_update_cat(ui, "Synthesized", ITEM_SYNTH_FIRST, ITEM_SYNTH_LAST);
    ui_tapes_update_cat(ui, "Passive", ITEM_PASSIVE_FIRST, ITEM_PASSIVE_LAST);
    ui_tapes_update_cat(ui, "Active", ITEM_ACTIVE_FIRST, ITEM_ACTIVE_LAST);
    ui_tapes_update_cat(ui, "Logistics", ITEM_LOGISTICS_FIRST, ITEM_LOGISTICS_LAST);

    if (select) ui_tree_select(&ui->tree, select);

    if (!ui_tapes_selected(ui)) {
        ui_panel_resize(&ui->panel, make_dim(
                        ui->tree_w + font->glyph_w,
                        ui->height));
        return;
    }

    ui_panel_resize(&ui->panel, make_dim(
                    ui->tree_w + ui->tape_w + font->glyph_w,
                    ui->height));

    enum item item = ui->tree.selected;
    const struct tech *tech = proxy_tech(render.proxy);
    const struct tape *tape = tapes_get(item);
    assert(tape);

    ui_str_set_item(&ui->name.str, item);

    im_lab_bits_update(&ui->lab_val, tech, item);
    ui_str_set_scaled(&ui->energy_val.str, tape_energy(tape));

    ui_str_set_item(&ui->host_val.str, tape_host(tape));

    ui_scroll_update(&ui->scroll, tape ? tape_len(tape) : 0);
}

static bool ui_tapes_event_user(struct ui_tapes *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_tapes_update(ui, 0);
        return false;
    }

    case EV_TAPES_TOGGLE: {
        if (ui_panel_is_visible(&ui->panel))
            ui_panel_hide(&ui->panel);
        else {
            ui_tapes_update(ui, 0);
            ui_panel_show(&ui->panel);
        }
        return false;
    }

    case EV_TAPE_SELECT: {
        enum item item = ((uintptr_t) ev->user.data1);
        ui_tapes_update(ui, item);
        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_STARS_TOGGLE:
    case EV_MODS_TOGGLE:
    case EV_MOD_SELECT:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
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

    render_push_event(EV_MAN_GOTO, link_to_u64(link), 0);
}

static bool ui_tapes_show_help(struct ui_tapes *ui)
{
    return
        item_is_active(ui->tree.selected) ||
        item_is_logistics(ui->tree.selected);
}

bool ui_tapes_event(struct ui_tapes *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_tapes_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if ((ret = ui_tree_event(&ui->tree, ev))) {
        if (ret == ui_action) ui_tapes_update(ui, 0);
        return true;
    }

    if (ui_tapes_selected(ui)) {
        if ((ret = ui_button_event(&ui->help, ev))) {
            ui_tapes_event_help(ui);
            return true;
        }

        if ((ret = ui_scroll_event(&ui->scroll, ev)))
            return true;

        if (ui_tapes_show_help(ui)) {
            if ((ret = ui_link_event(&ui->host_val, ev))) {
                enum item host = tape_host(tapes_get(ui->tree.selected));
                render_push_event(EV_TAPE_SELECT, host, 0);
                return true;
            }
        }
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

// \TODO: basically a copy of ui_tape. Need to not do that.
void ui_tapes_render_tape(
        struct ui_tapes *ui,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    struct font *font = ui_tapes_font();
    const struct tech *tech = proxy_tech(render.proxy);
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

void ui_tapes_render(struct ui_tapes *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_tree_render(&ui->tree, &layout, renderer);

    if (!ui_tapes_selected(ui)) return;

    struct font *font = ui_tapes_font();
    ui_layout_sep_x(&layout, font->glyph_w);
    struct ui_layout inner = ui_layout_inner(&layout);

    if (ui_tapes_show_help(ui)) {
        ui_layout_dir(&layout, ui_layout_left);
        ui_button_render(&ui->help, &layout, renderer);
        ui_layout_dir(&layout, ui_layout_right);
    }

    ui_label_render(&ui->name, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_y(&inner, font->glyph_h);

    ui_label_render(&ui->lab, &inner, renderer);
    im_lab_bits_render(&ui->lab_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_label_render(&ui->energy, &inner, renderer);
    ui_label_render(&ui->energy_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_y(&inner, font->glyph_h);

    ui_label_render(&ui->host, &inner, renderer);
    ui_link_render(&ui->host_val, &inner, renderer);
    ui_layout_next_row(&inner);

    ui_layout_sep_y(&inner, font->glyph_h);

    ui_label_render(&ui->tape, &inner, renderer);
    ui_layout_next_row(&inner);
    ui_tapes_render_tape(ui, &inner, renderer);
}
