/* ui.h
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/text.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct layout_dim { int w, h; };

struct layout_entry
{
    SDL_Rect rect;
    struct font *font;

    size_t rows, cols;
    struct layout_dim item;
};

struct layout
{
    SDL_Point pos;
    struct layout_dim bounds, bbox;

    size_t len;
    struct layout_entry entries[];
};

struct layout *layout_alloc(size_t entries, int max_width, int max_height);
void layout_free(struct layout *);

enum { layout_inf = 0 };
void layout_sep(struct layout *, int key);
void layout_rect(struct layout *, int key, int width, int height);
void layout_text(struct layout *, int key, struct font *, size_t cols, size_t rows);
void layout_list(struct layout *, int key, size_t rows, int width, int height);
void layout_grid(struct layout *, int key, size_t rows, size_t cols, int size);

void layout_finish(struct layout *, SDL_Point abs, SDL_Point rel);

struct layout_entry *layout_entry(struct layout *, int key);
SDL_Rect layout_abs(struct layout *, int key);

SDL_Point layout_entry_pos(struct layout_entry *);
SDL_Rect layout_entry_index(struct layout_entry *, size_t row, size_t col);
SDL_Point layout_entry_index_pos(struct layout_entry *, size_t row, size_t col);



// -----------------------------------------------------------------------------
// toggle
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
