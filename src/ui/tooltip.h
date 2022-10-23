/* tooltip.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "str.h"


// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip_style
{
    const struct font *font;
    struct rgba fg, bg, border;
    struct dim pad;
};

struct ui_tooltip
{
    struct ui_widget w;
    struct ui_tooltip_style s;
    struct ui_str str;

    SDL_Rect rect;
    bool disabled;
};

struct ui_tooltip ui_tooltip_new(struct ui_str, SDL_Rect);
void ui_tooltip_free(struct ui_tooltip *);

void ui_tooltip_show(struct ui_tooltip *);
void ui_tooltip_hide(struct ui_tooltip *);

enum ui_ret ui_tooltip_event(struct ui_tooltip *, const SDL_Event *);
void ui_tooltip_render(struct ui_tooltip *, SDL_Renderer *);
