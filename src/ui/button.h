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

enum ui_button_state
{
    ui_button_idle = 0,
    ui_button_hover,
    ui_button_pressed,
};

struct ui_button_style
{
    struct dim margin;

    struct
    {
        const struct font *font;
        struct rgba fg, bg;
    } idle, hover, pressed, disabled;

};

struct ui_button
{
    struct ui_widget w;
    struct ui_button_style s;
    struct ui_str str;

    bool disabled;
    enum ui_button_state state;
};

struct ui_button ui_button_new(struct ui_str);
struct ui_button ui_button_new_s(const struct ui_button_style *, struct ui_str);
void ui_button_free(struct ui_button *);
enum ui_ret ui_button_event(struct ui_button *, const SDL_Event *);
void ui_button_render(struct ui_button *, struct ui_layout *, SDL_Renderer *);
