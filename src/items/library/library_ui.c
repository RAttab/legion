/* library_ui.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// library
// -----------------------------------------------------------------------------

struct ui_library
{
    struct ui_label op, op_val;

    struct ui_label tape;
    struct ui_link tape_val;

    struct ui_label index, index_val, index_of, index_cap;

    struct ui_label value;
    struct ui_link value_val;

    struct im_library state;
};

static void *ui_library_alloc(void)
{
    struct ui_library *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_library) {
        .op = ui_label_new(ui_str_c("op:    ")),
        .op_val = ui_label_new(ui_str_v(12)),

        .tape = ui_label_new(ui_str_c("tape:  ")),
        .tape_val = ui_link_new(ui_str_v(item_str_len)),

        .index = ui_label_new(ui_str_c("index: ")),
        .index_val = ui_label_new(ui_str_v(3)),
        .index_of = ui_label_new(ui_str_c(" of ")),
        .index_cap = ui_label_new(ui_str_v(3)),

        .value = ui_label_new(ui_str_c("value: ")),
        .value_val = ui_link_new(ui_str_v(item_str_len)),
    };

    return ui;
}

static void ui_library_free(void *_ui)
{
    struct ui_library *ui = _ui;

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);

    ui_label_free(&ui->tape);
    ui_link_free(&ui->tape_val);

    ui_label_free(&ui->index);
    ui_label_free(&ui->index_val);
    ui_label_free(&ui->index_of);
    ui_label_free(&ui->index_cap);

    ui_label_free(&ui->value);
    ui_link_free(&ui->value_val);

    free(ui);
}

static void ui_library_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_library *ui = _ui;

    bool ok = chunk_copy(chunk, id, &ui->state, sizeof(ui->state));
    assert(ok);

    switch (ui->state.op) {

    case im_library_nil: {
        ui_set_nil(&ui->op_val);
        ui_set_nil(&ui->tape_val);
        ui_set_nil(&ui->index_val);
        ui_set_nil(&ui->index_cap);
        ui_set_nil(&ui->value_val);
        return;
    }

    case im_library_in:  {
        ui_str_setc(ui_set(&ui->op_val), "inputs");
        ui->op_val.s.fg = ui_st.rgba.in;
        break;
    }
    case im_library_out: {
        ui_str_setc(ui_set(&ui->op_val), "outputs");
        ui->op_val.s.fg = ui_st.rgba.out;
        break;
    }
    case im_library_req: {
        ui_str_setc(ui_set(&ui->op_val), "requirements");
        ui->op_val.s.fg = ui_st.rgba.work;
        break;
    }

    default: { assert(false); }
    }

    ui_str_set_item(ui_set(&ui->tape_val), ui->state.item);
    ui_str_set_u64(ui_set(&ui->index_val), ui->state.index);
    ui_str_set_u64(ui_set(&ui->index_cap), ui->state.len - 1);

    if (ui->state.value)
        ui_str_set_item(ui_set(&ui->value_val), ui->state.value);
    else ui_set_nil(&ui->value_val);
}

static bool ui_library_event(void *_ui, const SDL_Event *ev)
{
    struct ui_library *ui = _ui;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_link_event(&ui->tape_val, ev))) {
        if (ret != ui_action) return true;
        render_push_event(EV_TAPE_SELECT, ui->state.item, 0);
        return true;
    }

    if ((ret = ui_link_event(&ui->value_val, ev))) {
        if (ret != ui_action) return true;
        render_push_event(EV_TAPE_SELECT, ui->state.value, 0);
        return true;
    }

    return false;
}

static void ui_library_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_library *ui = _ui;

    ui_label_render(&ui->op, layout, renderer);
    ui_label_render(&ui->op_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->tape, layout, renderer);
    ui_link_render(&ui->tape_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->index, layout, renderer);
    ui_label_render(&ui->index_val, layout, renderer);
    ui_label_render(&ui->index_of, layout, renderer);
    ui_label_render(&ui->index_cap, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->value, layout, renderer);
    ui_link_render(&ui->value_val, layout, renderer);
    ui_layout_next_row(layout);
}
