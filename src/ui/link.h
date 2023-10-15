/* link.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct ui_link_style
{
    enum render_font font;
    struct { struct rgba fg, bg; } idle, hover, pressed, disabled;
};

void ui_link_style_default(struct ui_style *);

struct ui_link
{
    ui_widget w;
    struct ui_link_style s;
    struct ui_str str;

    bool disabled;
};

struct ui_link ui_link_new(struct ui_str);
void ui_link_free(struct ui_link *);
bool ui_link_event(struct ui_link *);
void ui_link_render(struct ui_link *, struct ui_layout *);
