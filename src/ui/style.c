/* style.c
   Rémi Attab (remi.attab@gmail.com), 30 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "style.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

struct ui_style ui_st = { 0 };

void ui_style_default(void)
{
    struct ui_style *s = &ui_st;

    // -------------------------------------------------------------------------
    // font
    // -------------------------------------------------------------------------

    { // font
        s->font.base = make_font(font_small, font_nil);
        s->font.bold = make_font(font_small, font_bold);
        s->font.dim = make_dim(
                s->font.base->glyph_w,
                s->font.base->glyph_h);
    }

    // -------------------------------------------------------------------------
    // rgba
    // -------------------------------------------------------------------------

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

        s->rgba.carret = rgba_gray_a(0xCC, 0xCC);

        s->rgba.box.bg = rgba_black();
        s->rgba.box.border = rgba_gray(0x33);

        s->rgba.list.hover = rgba_gray_a(0xFF, 0x22);
        s->rgba.list.selected = rgba_gray_a(0xFF, 0x11);

        s->rgba.link.idle.fg = make_rgba(0x22, 0x22, 0xFF, 0xFF);
        s->rgba.link.idle.bg = s->rgba.bg;
        s->rgba.link.hover.fg = make_rgba(0x00, 0x00, 0xCC, 0xFF);
        s->rgba.link.hover.bg = s->rgba.bg;
        s->rgba.link.pressed.fg = make_rgba(0x00, 0x00, 0x66, 0xFF);
        s->rgba.link.pressed.bg = s->rgba.bg;

        s->rgba.energy.consumed = make_rgba(0xFF, 0xD7, 0x00, 0xFF); // Gold
        s->rgba.energy.saved =    make_rgba(0xFF, 0x8C, 0x00, 0xFF); // SlateBlue
        s->rgba.energy.need =     make_rgba(0x8B, 0x00, 0x00, 0xFF); // DarkRed

        s->rgba.energy.stored =   make_rgba(0x00, 0x00, 0x80, 0xFF); // Navy
        s->rgba.energy.fusion =   make_rgba(0x00, 0x64, 0x00, 0xFF); // DarkGreen
        s->rgba.energy.solar =    make_rgba(0x80, 0x80, 0x00, 0xFF); // Olive
        s->rgba.energy.burner =   make_rgba(0x80, 0x00, 0x80, 0xFF); // Purple
        s->rgba.energy.kwheel =   make_rgba(0x4B, 0x00, 0x82, 0xFF); // Indigo
    }


    // -------------------------------------------------------------------------
    // pad
    // -------------------------------------------------------------------------

    { // pad
        s->pad.box = make_dim(6, 2);
    }


    // -------------------------------------------------------------------------
    // label
    // -------------------------------------------------------------------------

    { // label
        void fg(struct ui_label_style *label, struct rgba fg)
        {
            *label = s->label.base;
            label->fg = fg;
        }

        s->label.base = (struct ui_label_style) {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
            .disabled = s->rgba.disabled,
        };

        s->label.index = s->label.base;
        s->label.index.fg = rgba_gray(0x88);
        s->label.index.bg = rgba_gray_a(0x44, 0x88);

        s->label.bold = s->label.base;
        s->label.bold.font = s->font.bold;

        fg(&s->label.in, s->rgba.in);
        fg(&s->label.out, s->rgba.out);
        fg(&s->label.work, s->rgba.work);
        fg(&s->label.active, s->rgba.active);
        fg(&s->label.waiting, s->rgba.waiting);
        fg(&s->label.error, s->rgba.error);
        fg(&s->label.required, rgba_red());

        fg(&s->label.energy.consumed, s->rgba.energy.consumed);
        fg(&s->label.energy.stored, s->rgba.energy.stored);
        fg(&s->label.energy.need, s->rgba.energy.need);
        fg(&s->label.energy.saved, s->rgba.energy.saved);
        fg(&s->label.energy.fusion, s->rgba.energy.fusion);
        fg(&s->label.energy.solar, s->rgba.energy.solar);
        fg(&s->label.energy.burner, s->rgba.energy.burner);
        fg(&s->label.energy.kwheel, s->rgba.energy.kwheel);
    }


    // -------------------------------------------------------------------------
    // button
    // -------------------------------------------------------------------------

    { // button
        s->button.base = (struct ui_button_style) {
            .idle = {
                .font = s->font.base,
                .fg = s->rgba.fg,
                .bg = rgba_gray(0x22)
            },
            .hover = {
                .font = s->font.base,
                .fg = s->rgba.fg,
                .bg = rgba_gray(0x33)
            },
            .pressed = {
                .font = s->font.base,
                .fg = s->rgba.fg,
                .bg = rgba_gray(0x11)
            },
            .disabled = {
                .font = s->font.base,
                .fg = s->rgba.disabled,
                .bg = rgba_gray(0x22)
            },

            .margin = s->pad.box,
        };

        s->button.line = s->button.base;
        s->button.line.margin.h = 0;

        s->button.list.close = s->button.base;
        s->button.list.close.idle.bg = ui_st.rgba.bg;

        s->button.list.open = s->button.base;
        s->button.list.open.idle.font = ui_st.font.bold;
        s->button.list.open.hover.font = ui_st.font.bold;
        s->button.list.open.pressed.font = ui_st.font.bold;
        s->button.list.open.disabled.font = ui_st.font.bold;
    }


    // -------------------------------------------------------------------------
    // link
    // -------------------------------------------------------------------------

    s->link = (struct ui_link_style) {
        .font = s->font.base,
#define make_from(src) { .fg = (src).fg, .bg = (src).bg }
        .idle =     make_from(s->rgba.link.idle),
        .hover =    make_from(s->rgba.link.hover),
        .pressed =  make_from(s->rgba.link.pressed),
#undef make_from
        .disabled = { .fg = s->rgba.disabled, .bg = s->rgba.bg },
    };


    // -------------------------------------------------------------------------
    // tooltip
    // -------------------------------------------------------------------------

    s->tooltip = (struct ui_tooltip_style) {
        .font = s->font.base,
        .fg = s->rgba.fg,
        .bg = s->rgba.box.bg,
        .border = s->rgba.box.border,
        .pad = s->pad.box,
    };


    // -------------------------------------------------------------------------
    // scroll
    // -------------------------------------------------------------------------

    s->scroll = (struct ui_scroll_style) {
        .fg = rgba_gray(0x88),
        .bg = s->rgba.bg,
    };


    // -------------------------------------------------------------------------
    // input
    // -------------------------------------------------------------------------

    s->input = (struct ui_input_style) {
        .font = s->font.base,
        .fg = s->rgba.fg,
        .bg = s->rgba.bg,
        .border = s->rgba.box.border,
        .carret = s->rgba.carret,
        .pad = make_dim(2, 2),
    };


    // -------------------------------------------------------------------------
    // code
    // -------------------------------------------------------------------------

    s->code = (struct ui_code_style) {
        .font = s->font.base,
        .line = { .fg = s->label.index.fg, .bg = s->label.index.bg },
        .code = { .fg = s->rgba.fg, .bg = s->rgba.bg },
        .mark = make_rgba(0x00, 0xFF, 0x00, 0x66),
        .error = make_rgba(0xFF, 0x00, 0x00, 0x66),

        .breakpoint = {
            .fg = rgba_red(),
            .bg = make_rgba(0xFF, 0xFF, 0x00, 0x33),
            .hover = make_rgba(0xFF, 0xFF, 0x00, 0x88),
            .margin = 1,
        },

        .carret = s->rgba.carret,
    };


    // -------------------------------------------------------------------------
    // doc
    // -------------------------------------------------------------------------

    s->doc = (struct ui_doc_style) {
        .text = { .font = s->font.base, .fg = s->rgba.fg, .bg = s->rgba.bg },
        .bold = { .font = s->font.bold, .fg = s->rgba.fg, .bg = s->rgba.bg },

        .code = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = rgba_gray_a(0x66, 0x33),
        },

#define make_from(src) { .font = s->font.base, .fg = (src).fg, .bg = (src).bg }
        .link =    make_from(s->rgba.link.idle),
        .hover =   make_from(s->rgba.link.hover),
        .pressed = make_from(s->rgba.link.pressed),
#undef make_from

        .underline = { .fg = s->rgba.fg, .offset = 2 },
    };


    // -------------------------------------------------------------------------
    // list
    // -------------------------------------------------------------------------

    s->list = (struct ui_list_style) {
        .idle = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
        },

        .hover = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.hover,
        },

        .selected = {
            .font = s->font.bold,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.selected,
        },
    };


    // -------------------------------------------------------------------------
    // tree
    // -------------------------------------------------------------------------

    s->tree = (struct ui_tree_style) {
        .idle = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
        },

        .hover = {
            .font = s->font.base,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.hover,
        },

        .selected = {
            .font = s->font.bold,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.selected,
        },
    };


    // -------------------------------------------------------------------------
    // histo
    // -------------------------------------------------------------------------

    s->histo = (struct ui_histo_style) {
        .pad = make_dim(4, 4),
        .edge = make_rgba(0xFF, 0x00, 0x00, 0x33),
        .border = rgba_black(),

        .row = { .h = 8, .pad = 4 },

        .hover = {
            .fg = rgba_green(),
            .bg = ui_st.rgba.list.hover
        },

        .axes = {
            .pad = make_dim(1, 2),
            .fg = ui_st.rgba.fg
        },

        .value = {
            .font = ui_st.font.base,
            .fg = ui_st.rgba.fg,
            .bg = ui_st.rgba.bg,
        },
    };


    // -------------------------------------------------------------------------
    // panel
    // -------------------------------------------------------------------------

    s->panel = (struct ui_panel_style) {
        .margin = make_dim(2, 2),

        .bg = rgba_gray_a(0x11, 0x88),
        .border = s->rgba.box.border,

        .head = {
            .font = s->font.base,
            .fg = rgba_gray(0xAA),
            .bg = rgba_gray(0x11),
        },

        .focused = {
            .font = s->font.bold,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x22),
        },
    };

}
