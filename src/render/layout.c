/* layout.c
   Rémi Attab (remi.attab@gmail.com), 22 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "layout.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct layout *layout_alloc(size_t len, int width, int height)
{
    assert(width > 0);
    assert(height > 0);

    struct layout *layout =
        calloc(1, sizeof(*layout) + len * sizeof(layout->entries[0]));
    layout->bounds = (struct layout_dim) { .w = width, .h = height };
    layout->len = len;
    return layout;
}

void layout_free(struct layout *layout)
{
    free(layout);
}

static void layout_update(struct layout *layout, struct layout_entry *entry)
{
    if (entry->cols != layout_inf) {
        int width = entry->cols * entry->item.w;
        layout->bbox.w = u32_max(layout->bbox.w, width);
        assert(width <= layout->bounds.w);
    }

    if (entry->rows != layout_inf) {
        int height = entry->rows * entry->item.w;
        layout->bbox.h += height;
        assert(layout->bbox.h <= layout->bounds.h);
    }
}

void layout_sep(struct layout *layout, int key)
{
    struct layout_entry *entry = layout_entry(layout, key);
    entry->cols = layout_inf;
    entry->rows = 1;
    entry->item = (struct layout_dim) { .w = 1, .h = 10 };
    layout_update(layout, entry);
}

void layout_rect(struct layout *layout, int key, int width, int height)
{
    assert(width > 0 && height > 0);

    struct layout_entry *entry = layout_entry(layout, key);
    entry->cols = entry->rows = 1;
    entry->item = (struct layout_dim) { .w = width, .h = height };
    layout_update(layout, entry);
}

void layout_text(
        struct layout *layout, int key, struct font *font, size_t cols, size_t rows)
{
    assert(font);

    struct layout_entry *entry = layout_entry(layout, key);
    entry->font = font;
    entry->cols = cols;
    entry->rows = rows;
    entry->item = (struct layout_dim) { .w = font->glyph_w, .h = font->glyph_h };
    layout_update(layout, entry);
}

void layout_list(struct layout *layout, int key, size_t rows, int width, int height)
{
    assert(width > 0 && height > 0);

    struct layout_entry *entry = layout_entry(layout, key);
    entry->cols = 1;
    entry->rows = rows;
    entry->item = (struct layout_dim) { .w = width, .h = height };
    layout_update(layout, entry);
}

void layout_grid(struct layout *layout, int key, size_t rows, size_t cols, int size)
{
    assert(size > 0);

    struct layout_entry *entry = layout_entry(layout, key);
    entry->cols = cols;
    entry->rows = rows;
    entry->item = (struct layout_dim) { .w = size, .h = size };
    layout_update(layout, entry);
}

void layout_finish(struct layout *layout, SDL_Point rel)
{
    int y = rel.y;
    for (size_t i = 0; i < layout->len; ++i) {
        struct layout_entry *entry = &layout->entries[i];

        if (entry->cols == layout_inf)
            entry->cols = layout->bounds.w / entry->item.w;
        if (entry->rows == layout_inf) {
            entry->rows = (layout->bounds.h - y) / entry->item.h;
            layout->bbox.h = 0; // can only allow a single inf rows.
        }

        entry->rect = (SDL_Rect) {
            .x = rel.x, .y = y,
            .w = entry->item.w * entry->cols,
            .h = entry->item.h * entry->rows,
        };

        assert(entry->rect.y + entry->rect.h <= layout->bounds.h);
        y += entry->rect.h;
    }

    layout->bbox.h = y;
}


struct layout_entry *layout_entry(struct layout *layout, int key)
{
    assert(key >= 0 && (unsigned) key < layout->len);
    struct layout_entry *entry = &layout->entries[key];
    if (entry->font) font_reset(entry->font);
    return entry;
}

SDL_Rect layout_abs(struct layout *layout, int key)
{
    struct layout_entry *entry = layout_entry(layout, key);
    return (SDL_Rect) {
        .x = layout->pos.x + entry->rect.x,
        .y = layout->pos.y + entry->rect.y,
        .w = entry->rect.w, .h = entry->rect.h,
    };
}


SDL_Point layout_entry_pos(struct layout_entry *entry)
{
    return (SDL_Point) { .x = entry->rect.x, entry->rect.y };
}

SDL_Rect layout_entry_index(struct layout_entry *entry, size_t row, size_t col)
{
    assert(row < entry->rows);
    assert(col < entry->cols);

    return (SDL_Rect) {
        .x = entry->rect.x + col * entry->item.w,
        .y = entry->rect.y + row * entry->item.h,
        .w = entry->item.w * (col == layout_inf ? entry->cols : 1),
        .h = entry->item.h * (row == layout_inf ? entry->rows : 1),
    };
}

SDL_Point layout_entry_index_pos(struct layout_entry *entry, size_t row, size_t col)
{
    assert(row < entry->rows);
    assert(col < entry->cols);

    return (SDL_Point) {
        .x = entry->rect.x + col * entry->item.w,
        .y = entry->rect.y + row * entry->item.h,
    };
}

