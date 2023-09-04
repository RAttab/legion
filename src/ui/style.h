/* style.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"

// All the relevant includes are taken care off in ui.h

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

extern struct ui_style
{
    struct {
        struct dim dim;
        const struct font *base, *bold;
    } font;

    struct
    {
        struct rgba fg, bg;
        struct rgba in, out, work;
        struct rgba error, warn, info;
        struct rgba waiting, working;
        struct rgba active, disabled;
        struct rgba carret;
        struct { struct rgba bg, border; } box;
        struct { struct rgba hover, selected; } list;
        struct { struct { struct rgba fg, bg; } idle, hover, pressed; } link;
        struct { struct rgba queue, work, clean, fail, idle; } worker;
        struct {
            struct rgba consumed, saved, need;
            struct rgba stored, fusion, solar, burner, kwheel, battery;
        } energy;
    } rgba;

    struct
    {
        struct dim box;
    } pad;

    struct
    {
        struct ui_label_style base, bold;
        struct ui_label_style index;
        struct ui_label_style in, out, work;
        struct ui_label_style active, waiting, error;
        struct ui_label_style required;
        struct {
            struct ui_label_style consumed, saved, need;
            struct ui_label_style stored, fusion, solar, burner, kwheel;
        } energy;
    } label;

    struct
    {
        struct ui_button_style base;
        struct ui_button_style line;
        struct { struct ui_button_style close, open; } list;
    } button;

    struct ui_link_style link;
    struct ui_tooltip_style tooltip;
    struct ui_scroll_style scroll;
    struct ui_input_style input;
    struct ui_code_style code;
    struct ui_doc_style doc;
    struct ui_list_style list;
    struct ui_tree_style tree;
    struct ui_histo_style histo;
    struct ui_tabs_style tabs;
    struct ui_panel_style panel;

} ui_st;

void ui_style_default(void);
