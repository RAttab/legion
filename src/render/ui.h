/* ui.h
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/text.h"

#include "SDL.h"

// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

enum ui_ret
{
    ui_nil = 0,
    ui_action = 1 << 0,
    ui_consume = 1 << 1,
    ui_invalidate = 1 << 2,
};


// -----------------------------------------------------------------------------
// toggle
// -----------------------------------------------------------------------------

enum { ui_toggle_layout_cols = 2 };

struct ui_toggle
{
    struct SDL_Rect rect;

    char str[text_line_cap];
    size_t str_len;

    bool hover;
    bool selected;
    bool disabled;
};

void ui_toggle_size(struct font *, size_t str_len, int *width, int *height);
void ui_toggle_init(
        struct ui_toggle *, const struct SDL_Rect *, const char *str, size_t len);

void ui_toggle_render(struct ui_toggle *, SDL_Renderer *, SDL_Point, struct font *);
enum ui_ret ui_toggle_events(struct ui_toggle *, SDL_Event *);


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

enum { ui_scroll_layout_cols = 1 };

struct ui_scroll
{
    struct SDL_Rect bar;
    struct SDL_Rect events;

    size_t total;
    size_t first;
    size_t visible;

    struct {
        int y;
        int top;
    } drag;
};

void ui_scroll_init(
        struct ui_scroll *,
        const SDL_Rect *bar_abs,
        const SDL_Rect *events_abs,
        size_t total, size_t visible);

void ui_scroll_update(struct ui_scroll *, size_t total);
void ui_scroll_render(struct ui_scroll *, SDL_Renderer *, SDL_Point);
enum ui_ret ui_scroll_events(struct ui_scroll *, SDL_Event *);


// -----------------------------------------------------------------------------
// click
// -----------------------------------------------------------------------------

struct ui_click
{
    struct SDL_Rect rect;

    bool hover;
    bool disabled;
};

void ui_click_init(struct ui_click *, SDL_Rect);
void ui_click_render(struct ui_click *, SDL_Renderer *, SDL_Point);
enum ui_ret ui_click_events(struct ui_click *, SDL_Event *);
