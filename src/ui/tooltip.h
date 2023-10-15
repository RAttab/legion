/* tooltip.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip_style
{
    enum render_font font;
    struct rgba fg, bg, border;
    struct dim pad;
};

void ui_tooltip_style_default(struct ui_style *);

void ui_tooltip_init(void);
void ui_tooltip_free(void);

void ui_tooltip_set(struct rect, struct ui_str);
void ui_tooltip_unset(void);

void ui_tooltip_render(void);
