/* style.c
   Rémi Attab (remi.attab@gmail.com), 30 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

struct ui_style ui_st = { 0 };

void ui_style_default(void)
{
    struct ui_style *s = &ui_st;

    s->font = font_mono6;

    { // rgba
        s->rgba.fg = rgba_white();
        s->rgba.bg = rgba_nil();

        s->rgba.in = rgba_green();
        s->rgba.out = rgba_blue();
        s->rgba.work = rgba_yellow();

        s->rgba.error = rgba_red();
        s->rgba.warn = rgba_yellow();
        s->rgba.info = rgba_white();

        s->rgba.waiting = rgba_blue();
        s->rgba.working = rgba_yellow();

        s->rgba.active = rgba_green();
        s->rgba.disabled = rgba_gray(0x88);

        s->rgba.carret = rgba_gray_a(0xCC, 0x88);

        s->rgba.box.bg = rgba_black();
        s->rgba.box.border = rgba_gray(0x33);
    }

    { // pad
        s->pad.box = make_dim(6, 2);
    }

    { // label
        void fg(struct ui_label_style *label, struct rgba fg)
        {
            *label = s->label.base;
            label->fg = fg;
        }

        s->label.base = (struct ui_label_style) {
            .font = s->font,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
            .disabled = s->rgba.disabled,
        };

        s->label.index = s->label.base;
        s->label.index.fg = rgba_gray(0x88);
        s->label.index.bg = rgba_gray_a(0x44, 0x88);

        s->label.title = s->label.base;
        s->label.title.bg = rgba_gray_a(0x44, 0x44);

        fg(&s->label.in, s->rgba.in);
        fg(&s->label.out, s->rgba.out);
        fg(&s->label.work, s->rgba.work);
        fg(&s->label.active, s->rgba.active);
        fg(&s->label.waiting, s->rgba.waiting);
        fg(&s->label.error, s->rgba.error);
    }

    { // button
        s->button.base = (struct ui_button_style) {
            .font = s->font,
            .idle =     { .fg = ui_st.rgba.fg,       .bg = rgba_gray(0x22) },
            .hover =    { .fg = ui_st.rgba.fg,       .bg = rgba_gray(0x33) },
            .pressed =  { .fg = ui_st.rgba.fg,       .bg = rgba_gray(0x11) },
            .disabled = { .fg = ui_st.rgba.disabled, .bg = rgba_gray(0x22) },
            .pad = ui_st.pad.box,
        };

        s->button.line = s->button.base;
        s->button.line.pad.h = 0;
    }

    s->link = (struct ui_link_style) {
        .font = s->font,
        .idle =     { .fg = ui_st.rgba.fg,       .bg = ui_st.rgba.bg },
        .hover =    { .fg = ui_st.rgba.fg,       .bg = rgba_gray(0x33) },
        .pressed =  { .fg = ui_st.rgba.fg,       .bg = rgba_gray(0x11) },
        .disabled = { .fg = ui_st.rgba.disabled, .bg = ui_st.rgba.bg },
    };

    s->tooltip = (struct ui_tooltip_style) {
        .font = s->font,
        .fg = ui_st.rgba.fg,
        .bg = ui_st.rgba.box.bg,
        .border = ui_st.rgba.box.border,
        .pad = ui_st.pad.box,
    };

    s->scroll = (struct ui_scroll_style) {
        .fg = rgba_gray(0x88),
        .bg = ui_st.rgba.bg,
    };

    s->input = (struct ui_input_style) {
        .font = s->font,
        .fg = ui_st.rgba.fg,
        .bg = ui_st.rgba.bg,
        .border = ui_st.rgba.box.border,
        .carret = ui_st.rgba.carret,
        .pad = make_dim(2, 2),
    };
}
