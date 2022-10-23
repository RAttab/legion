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

enum ui_panel_state
{
    ui_panel_hidden = 0,
    ui_panel_visible = 1,
    ui_panel_focused = 2,
};

struct ui_panel_style
{
    struct dim margin;
    struct rgba bg, border;
    struct {
        const struct font *font;
        struct rgba fg, bg;
    } head, focused;
};

struct ui_panel
{
    struct ui_widget w;
    struct ui_panel_style s;
    struct ui_layout layout;

    struct ui_label title;
    struct ui_button close;

    bool menu;
    enum ui_panel_state state;
};

struct ui_panel *ui_panel_current(void);

struct ui_panel *ui_panel_menu(struct pos, struct dim);
struct ui_panel *ui_panel_title(struct pos, struct dim, struct ui_str);
void ui_panel_free(struct ui_panel *);

void ui_panel_resize(struct ui_panel *, struct dim);
void ui_panel_focus(struct ui_panel *);
void ui_panel_show(struct ui_panel *);
void ui_panel_hide(struct ui_panel *);
bool ui_panel_is_visible(struct ui_panel *);

enum ui_ret ui_panel_event(struct ui_panel *, const SDL_Event *);
enum ui_ret ui_panel_event_consume(struct ui_panel *, const SDL_Event *);
struct ui_layout ui_panel_render(struct ui_panel *, SDL_Renderer *);
