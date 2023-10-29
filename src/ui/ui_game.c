/* ui_game.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// waiting
// -----------------------------------------------------------------------------

struct ui_label ui_waiting_new(void)
{
    return ui_label_new(ui_str_v(8));
}

void ui_waiting_idle(struct ui_label *label)
{
    label->disabled = true;
    ui_str_setc(&label->str, "idle");
}

void ui_waiting_set(struct ui_label *label, bool waiting)
{
    label->disabled = false;

    if (waiting) {
        ui_str_setc(&label->str, "waiting");
        label->s.fg = ui_st.rgba.waiting;
    }
    else {
        ui_str_setc(&label->str, "working");
        label->s.fg = ui_st.rgba.working;
    }
}


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

struct ui_label ui_loops_new(void)
{
    return ui_label_new(ui_str_v(3));
}

void ui_loops_set(struct ui_label *label, im_loops loops)
{
    label->disabled = false;

    if (loops != im_loops_inf)
        ui_str_set_u64(&label->str, loops);
    else ui_str_setc(&label->str, "inf");
}


// -----------------------------------------------------------------------------
// lab bits
// -----------------------------------------------------------------------------

void ui_lab_bits_style_default(struct ui_style *s)
{
    struct dim cell = engine_cell();
    s->lab_bits = (struct ui_lab_bits_style) {
        .margin = make_dim(cell.w / 2, cell.w / 2),
        .fg = rgba_gray(0xAA),
        .border = s->rgba.box.border,
    };
}

struct ui_lab_bits ui_lab_bits_new(void)
{
    return (struct ui_lab_bits) { .s = ui_st.lab_bits };
}

void ui_lab_bits_update(
        struct ui_lab_bits *ui, const struct tech *tech, enum item item)
{
    if (!item)
        ui->bits = ui->known = 0;
    else {
        ui->bits = specs_var_assert(make_spec(item, spec_lab_bits));
        ui->known = tech_learned_bits(tech, item);
    }
}

void ui_lab_bits_render(struct ui_lab_bits *ui, struct ui_layout *layout)
{
    if (!ui->bits) return;

    ui_widget w = make_ui_widget(make_dim(ui_layout_inf, engine_cell().h));
    ui_layout_add(layout, &w);

    unit bits_w = (w.w - (ui->s.margin.w * 2)) / ui->bits;
    unit outer_w = (ui->bits * bits_w) + ui->s.margin.w * 2;

    const render_layer layer = render_layer_push(1);
    render_rect_line(layer, ui->s.border, make_rect(w.x, w.y, outer_w, w.h));

    struct rect bits = {
        .x = w.x + ui->s.margin.w,
        .y = w.y + ui->s.margin.h,
        .w = bits_w,
        .h = w.h - (ui->s.margin.h * 2),
    };

    for (size_t i = 0; i < ui->bits; ++i, bits.x += bits.w) {
        if (!(ui->known & (1ULL << i))) continue;
        render_rect_fill(layer, ui->s.fg, bits);
    }

    render_layer_pop();
}
