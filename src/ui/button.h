/* button.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "str.h"


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------


struct ui_button_style
{
    int16_t height;
    struct dim margin;

    struct
    {
        enum render_font font;
        struct rgba fg, bg;
    } idle, hover, pressed, disabled;

    enum ui_align align;
};

void ui_button_style_default(struct ui_style *);


struct ui_button
{
    ui_widget w;
    struct ui_button_style s;
    struct ui_str str;
    bool disabled;
};

struct ui_button ui_button_new(struct ui_str);
struct ui_button ui_button_new_s(const struct ui_button_style *, struct ui_str);
void ui_button_free(struct ui_button *);
bool ui_button_event(struct ui_button *);
void ui_button_render(struct ui_button *, struct ui_layout *);
