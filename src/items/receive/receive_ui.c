/* receive_ui.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// receive
// -----------------------------------------------------------------------------

struct ui_receive
{
    struct font *font;

    struct ui_label target, target_val;
    struct ui_label channel, channel_val;

    struct ui_label buffer, buffer_len, buffer_sep, buffer_cap;
    struct ui_label packet, packet_data;

    uint8_t len, cap;
    struct im_packet packets[im_receive_buffer_max];
};

static void *ui_receive_alloc(struct font *font)
{
    struct ui_receive *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_receive) {
        .font = font,

        .target = ui_label_new(font, ui_str_c("target: ")),
        .target_val = ui_label_new(font, ui_str_v(coord_str_len)),
        .channel = ui_label_new(font, ui_str_c("channel: ")),
        .channel_val = ui_label_new(font, ui_str_v(1)),

        .buffer = ui_label_new(font, ui_str_c("buffer: ")),
        .buffer_sep = ui_label_new(font, ui_str_c(" of ")),
        .buffer_len = ui_label_new(font, ui_str_v(1)),
        .buffer_cap = ui_label_new(font, ui_str_v(1)),

        .packet = ui_label_new(font, ui_str_v(1)),
        .packet_data = ui_label_new(font, ui_str_v(16)),
    };

    ui->packet.fg = rgba_gray(0x88);
    ui->packet.bg = rgba_gray_a(0x44, 0x88);

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

static void ui_receive_update(void *_ui, struct chunk *chunk, id_t id)
{
    struct ui_receive *ui = _ui;

    const struct im_receive *receive = chunk_get(chunk, id);
    assert(receive);

    ui_str_set_coord(&ui->target_val.str, receive->target);
    ui_str_set_u64(&ui->channel_val.str, receive->channel);

    ui->cap = im_receive_cap(receive);
    ui_str_set_u64(&ui->buffer_cap.str, ui->cap);

    ui->len = receive->head - receive->tail;
    ui_str_set_u64(&ui->buffer_len.str, ui->len);

    for (size_t i = 0; i < ui->len; ++i)
        ui->packets[i] = receive->buffer[(receive->tail + i) % ui->cap];
}

static void ui_receive_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_receive *ui = _ui;

    ui_label_render(&ui->target, layout, renderer);
    ui_label_render(&ui->target_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->channel, layout, renderer);
    ui_label_render(&ui->channel_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->buffer, layout, renderer);
    ui_label_render(&ui->buffer_len, layout, renderer);
    ui_label_render(&ui->buffer_sep, layout, renderer);
    ui_label_render(&ui->buffer_cap, layout, renderer);
    ui_layout_next_row(layout);

    for (size_t i = 0; i < ui->len; ++i) {
        const struct im_packet *packet = ui->packets + i;
        ui_str_set_u64(&ui->packet.str, i);

        for (size_t j = 0; j < packet->len; ++j) {
            ui_label_render(&ui->packet, layout, renderer);
            ui_layout_sep_x(layout, ui->font->glyph_w);

            ui_str_set_hex(&ui->packet_data.str, packet->data[j]);
            ui_label_render(&ui->packet_data, layout, renderer);

            ui_str_setc(&ui->packet.str, " ");
            ui_layout_next_row(layout);
        }

        ui_layout_sep_y(layout, ui->font->glyph_h);
    }
}
