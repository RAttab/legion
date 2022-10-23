/* link.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "str.h"


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

enum ui_link_state
{
    ui_link_idle = 0,
    ui_link_hover,
    ui_link_pressed,
};


struct ui_link_style
{
    const struct font *font;
    struct { struct rgba fg, bg; } idle, hover, pressed, disabled;
};

void ui_link_style_default(struct ui_style *);

struct ui_link
{
    struct ui_widget w;
    struct ui_link_style s;
    struct ui_str str;

    enum ui_link_state state;
    bool disabled;
};

struct ui_link ui_link_new(struct ui_str);
void ui_link_free(struct ui_link *);
enum ui_ret ui_link_event(struct ui_link *, const SDL_Event *);
void ui_link_render(struct ui_link *, struct ui_layout *, SDL_Renderer *);
