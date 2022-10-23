/* layout.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

enum ui_layout_dir { ui_layout_right, ui_layout_left };

struct ui_layout
{
    struct
    {
        struct pos pos;
        struct dim dim;
    } base, row;

    enum ui_layout_dir dir;
};

enum { ui_layout_inf = -1 };

struct ui_layout ui_layout_new(struct pos, struct dim);
void ui_layout_resize(struct ui_layout *, struct pos, struct dim);
void ui_layout_add(struct ui_layout *, struct ui_widget *);

struct ui_layout ui_layout_inner(struct ui_layout *);
struct ui_layout ui_layout_split_x(struct ui_layout *, int16_t w);

void ui_layout_sep_x(struct ui_layout *, int16_t px);
void ui_layout_sep_col(struct ui_layout *);
void ui_layout_sep_cols(struct ui_layout *, size_t n);
void ui_layout_tab(struct ui_layout *, size_t n);
void ui_layout_mid(struct ui_layout *, int width);

void ui_layout_sep_y(struct ui_layout *, int16_t px);
void ui_layout_sep_row(struct ui_layout *);
void ui_layout_next_row(struct ui_layout *);

void ui_layout_dir(struct ui_layout *, enum ui_layout_dir);

inline bool ui_layout_is_nil(struct ui_layout *layout)
{
    return pos_is_nil(layout->base.pos) && dim_is_nil(layout->base.dim);
}
