/* receive_ui.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// receive
// -----------------------------------------------------------------------------

struct ui_receive
{
    struct ui_label target, target_val;
    struct ui_label channel, channel_val;

    struct ui_label buffer, buffer_len, buffer_sep, buffer_cap;
    struct ui_label packet, packet_data;

    uint8_t len, cap;
    struct im_packet packets[im_receive_buffer_max];
};

static void *ui_receive_alloc(void)
{
    struct ui_receive *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_receive) {
        .target = ui_label_new(ui_str_c("target: ")),
        .target_val = ui_label_new(ui_str_v(symbol_cap)),
        .channel = ui_label_new(ui_str_c("channel: ")),
        .channel_val = ui_label_new(ui_str_v(1)),

        .buffer = ui_label_new(ui_str_c("buffer: ")),
        .buffer_sep = ui_label_new(ui_str_c(" of ")),
        .buffer_len = ui_label_new(ui_str_v(1)),
        .buffer_cap = ui_label_new(ui_str_v(1)),

        .packet = ui_label_new_s(&ui_st.label.index, ui_str_v(1)),
        .packet_data = ui_label_new(ui_str_v(16)),
    };

    return ui;
}

static void ui_receive_free(void *_ui)
{
    struct ui_receive *ui = _ui;

    ui_label_free(&ui->target);
    ui_label_free(&ui->target_val);
    ui_label_free(&ui->channel);
    ui_label_free(&ui->channel_val);

    ui_label_free(&ui->buffer);
    ui_label_free(&ui->buffer_sep);
    ui_label_free(&ui->buffer_len);
    ui_label_free(&ui->buffer_cap);

    ui_label_free(&ui->packet);
    ui_label_free(&ui->packet_data);

    free(ui);
}

static void ui_receive_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_receive *ui = _ui;

    const struct im_receive *receive = chunk_get(chunk, id);
    assert(receive);

    if (coord_is_nil(receive->target)) ui_set_nil(&ui->target_val);
    else ui_str_set_coord_name(ui_set(&ui->target_val), receive->target);

    ui_str_set_u64(&ui->channel_val.str, receive->channel);

    ui->cap = im_receive_cap(receive);
    ui_str_set_u64(&ui->buffer_cap.str, ui->cap);

    ui->len = receive->head - receive->tail;
    ui_str_set_u64(&ui->buffer_len.str, ui->len);

    for (size_t i = 0; i < ui->len; ++i)
        ui->packets[i] = receive->buffer[(receive->tail + i) % ui->cap];
}

static void ui_receive_render(void *_ui, struct ui_layout *layout)
{
    struct ui_receive *ui = _ui;

    ui_label_render(&ui->target, layout);
    ui_label_render(&ui->target_val, layout);
    ui_layout_next_row(layout);
    ui_label_render(&ui->channel, layout);
    ui_label_render(&ui->channel_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->buffer, layout);
    ui_label_render(&ui->buffer_len, layout);
    ui_label_render(&ui->buffer_sep, layout);
    ui_label_render(&ui->buffer_cap, layout);
    ui_layout_next_row(layout);

    for (size_t i = 0; i < ui->len; ++i) {
        const struct im_packet *packet = ui->packets + i;
        ui_str_set_u64(&ui->packet.str, i);

        for (size_t j = 0; j < packet->len; ++j) {
            ui_label_render(&ui->packet, layout);
            ui_layout_sep_col(layout);

            ui_str_set_hex(&ui->packet_data.str, packet->data[j]);
            ui_label_render(&ui->packet_data, layout);

            ui_str_setc(&ui->packet.str, " ");
            ui_layout_next_row(layout);
        }

        ui_layout_sep_row(layout);
    }
}
