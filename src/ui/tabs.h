/* tabs.h
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "button.h"

// -----------------------------------------------------------------------------
// tabs
// -----------------------------------------------------------------------------

struct ui_tabs_style
{
    struct dim pad;
    const struct font *font, *bold;
    struct rgba fg, line, hover, pressed;
};

void ui_tabs_style_default(struct ui_style *);

struct ui_tab
{
    uint64_t user;
    struct rgba fg;
    struct ui_str str;

    bool hidden;
    int16_t x, w;
};

struct ui_tabs
{
    struct ui_widget w;
    struct ui_tabs_style s;

    struct ui_str str;

    bool update;
    struct { bool show; int16_t x, w; } left, right;
    struct { bool update; uint64_t user; } select;
    struct { bool show; uint64_t user; } close;

    size_t len, cap, first;
    struct ui_tab *list;
};

struct ui_tabs ui_tabs_new(size_t chars, bool close);
void ui_tabs_free(struct ui_tabs *);

void ui_tabs_clear(struct ui_tabs *);
void ui_tabs_select(struct ui_tabs *, uint64_t user);
uint64_t ui_tabs_selected(struct ui_tabs *);
uint64_t ui_tabs_closed(struct ui_tabs *);

void ui_tabs_reset(struct ui_tabs *);
struct ui_str *ui_tabs_add(struct ui_tabs *, uint64_t user);
struct ui_str *ui_tabs_add_s(struct ui_tabs *, uint64_t user, struct rgba fg);

enum ui_ret ui_tabs_event(struct ui_tabs *, const SDL_Event *);
void ui_tabs_render(struct ui_tabs *, struct ui_layout *, SDL_Renderer *);
