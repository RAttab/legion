/* layout.h
   Rémi Attab (remi.attab@gmail.com), 22 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "SDL.h"

struct font;


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

static const size_t layout_inf = -1;
void layout_sep(struct layout *, int key);
void layout_rect(struct layout *, int key, int width, int height);
void layout_text(struct layout *, int key, struct font *, size_t cols, size_t rows);
void layout_list(struct layout *, int key, size_t rows, int width, int height);
void layout_grid(struct layout *, int key, size_t rows, size_t cols, int size);

void layout_finish(struct layout *, SDL_Point rel);

struct layout_entry *layout_entry(struct layout *, int key);

SDL_Rect layout_abs(struct layout *, int key);
SDL_Rect layout_abs_index(struct layout *, int key, size_t row, size_t col);
SDL_Rect layout_abs_rect(
        struct layout *, int key, size_t row, size_t col, size_t w, size_t h);

SDL_Point layout_entry_pos(struct layout_entry *);
SDL_Rect layout_entry_index(struct layout_entry *, size_t row, size_t col);
SDL_Point layout_entry_index_pos(struct layout_entry *, size_t row, size_t col);
SDL_Rect layout_entry_rect(
        struct layout_entry *, size_t row, size_t col, size_t w, size_t h);

void layout_entry_point(struct layout_entry *, SDL_Point pos, size_t *row, size_t *col);
