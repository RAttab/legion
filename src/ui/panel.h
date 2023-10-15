/* panel.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "label.h"
#include "button.h"


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

struct ui_panel_style
{
    struct dim margin;
    struct rgba bg, border;
    struct { enum render_font font; struct rgba fg, bg; } head, focused;
};

void ui_panel_style_default(struct ui_style *);


struct ui_panel
{
    ui_widget w;
    struct ui_panel_style s;

    struct ui_label title;
    struct ui_button close;

    bool menu, visible;
};

struct ui_panel *ui_panel_current(void);

struct ui_panel *ui_panel_menu(struct dim);
struct ui_panel *ui_panel_title(struct dim, struct ui_str);
void ui_panel_free(struct ui_panel *);

void ui_panel_resize(struct ui_panel *, struct dim);
void ui_panel_focus(struct ui_panel *);
void ui_panel_show(struct ui_panel *);
void ui_panel_hide(struct ui_panel *);

enum ui_panel_ev { ui_panel_ev_nil = 0, ui_panel_ev_skip, ui_panel_ev_close };
enum ui_panel_ev ui_panel_event(struct ui_panel *);
void ui_panel_event_consume(struct ui_panel *);

struct ui_layout ui_panel_render(struct ui_panel *, struct ui_layout *);
