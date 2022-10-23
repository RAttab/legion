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

    { // font
        s->font.base = make_font(font_small, font_nil);
        s->font.bold = make_font(font_small, font_bold);
        s->font.dim = make_dim(
                s->font.base->glyph_w,
                s->font.base->glyph_h);
    }

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

        s->rgba.worker.queue = make_rgba(0x00, 0x00, 0x80, 0xFF); // Navy
        s->rgba.worker.work =  make_rgba(0xFF, 0xD7, 0x00, 0xFF); // Gold
        s->rgba.worker.clean = make_rgba(0x80, 0x80, 0x00, 0xFF); // Olive
        s->rgba.worker.fail =  make_rgba(0x8B, 0x00, 0x00, 0xFF); // DarkRed
        s->rgba.worker.idle =  make_rgba(0x80, 0x00, 0x80, 0xFF); // Purple

        s->rgba.energy.consumed = make_rgba(0xFF, 0xD7, 0x00, 0xFF); // Gold
        s->rgba.energy.saved =    make_rgba(0xFF, 0x8C, 0x00, 0xFF); // SlateBlue
        s->rgba.energy.need =     make_rgba(0x8B, 0x00, 0x00, 0xFF); // DarkRed
        s->rgba.energy.stored =   make_rgba(0x00, 0x00, 0x80, 0xFF); // Navy
        s->rgba.energy.fusion =   make_rgba(0x00, 0x64, 0x00, 0xFF); // DarkGreen
        s->rgba.energy.solar =    make_rgba(0x80, 0x80, 0x00, 0xFF); // Olive
        s->rgba.energy.burner =   make_rgba(0x80, 0x00, 0x80, 0xFF); // Purple
        s->rgba.energy.kwheel =   make_rgba(0x4B, 0x00, 0x82, 0xFF); // Indigo
    }

    { // pad
        s->pad.box = make_dim(6, 2);
    }

    ui_label_style_default(s);
    ui_button_style_default(s);
    ui_link_style_default(s);
    ui_tooltip_style_default(s);
    ui_scroll_style_default(s);
    ui_input_style_default(s);
    ui_code_style_default(s);
    ui_doc_style_default(s);
    ui_list_style_default(s);
    ui_tree_style_default(s);
    ui_histo_style_default(s);
    ui_panel_style_default(s);
}
