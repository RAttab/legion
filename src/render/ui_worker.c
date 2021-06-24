/* ui_worker.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct ui_worker
{
    struct ui_label src, src_val;
    struct ui_label dst, dst_val;
    struct ui_label item, item_val;
    struct ui_label state, state_val;
};

static void ui_worker_init(struct ui_worker *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_worker) {
        .src = ui_label_new(font, ui_str_c("src:   ")),
        .src_val = ui_label_new(font, ui_str_v(id_str_len)),

        .dst = ui_label_new(font, ui_str_c("dst:   ")),
        .dst_val = ui_label_new(font, ui_str_v(id_str_len)),

        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(vm_atom_cap)),
    };
}

static void ui_worker_free(struct ui_worker *ui)
{
    ui_label_free(&ui->src);
    ui_label_free(&ui->src_val);
    ui_label_free(&ui->dst);
    ui_label_free(&ui->dst_val);
    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);
}

static void ui_worker_update(struct ui_worker *ui, struct worker *state)
{
    if (!state->src) ui_str_setc(&ui->src_val.str, "nil");
    else ui_str_set_id(&ui->src_val.str, state->src);

    if (!state->dst) ui_str_setc(&ui->dst_val.str, "nil");
    else ui_str_set_id(&ui->dst_val.str, state->dst);

    if (!state->item) ui_str_setc(&ui->item_val.str, "nil");
    else ui_str_set_hex(&ui->item_val.str, state->item);
}

static bool ui_worker_event(
        struct ui_worker *ui, struct worker *state, const SDL_Event *ev)
{
    (void) ui, (void) state, (void) ev;
    return false;
}

static void ui_worker_render(
        struct ui_worker *ui, struct worker *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    (void) state;
    
    ui_label_render(&ui->src, layout, renderer);
    ui_label_render(&ui->src_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->dst, layout, renderer);
    ui_label_render(&ui->dst_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);
}
