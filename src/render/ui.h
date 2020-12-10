/* ui.h
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/text.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// selectable
// -----------------------------------------------------------------------------

enum ui_toggle_ret
{
    ui_toggle_nil = 0,
    ui_toggle_flip = 1 << 0,
    ui_toggle_consume = 1 << 1,
    ui_toggle_invalidate = 1 << 2,
};

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
enum ui_toggle_ret ui_toggle_events(struct ui_toggle *, SDL_Event *);
