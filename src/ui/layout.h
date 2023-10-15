/* layout.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

enum ui_layout_dir : uint8_t
{
    ui_layout_hori = 0U,
    ui_layout_hori_mask = 1U << ui_layout_hori,
    ui_layout_left_right = 0U << ui_layout_hori,
    ui_layout_right_left = 1U << ui_layout_hori,

    ui_layout_vert = 1U,
    ui_layout_vert_mask = 1U << ui_layout_vert,
    ui_layout_up_down = 0U << ui_layout_vert,
    ui_layout_down_up = 1U << ui_layout_vert,
};

struct ui_layout
{
    struct
    {
        struct pos pos;
        struct dim dim;
    } base, row;

    enum ui_layout_dir dir;
};

constexpr unit ui_layout_inf = -1;

struct ui_layout ui_layout_new(struct pos, struct dim);
void ui_layout_resize(struct ui_layout *, struct dim);

struct dim  ui_layout_remaining(struct ui_layout *);
void ui_layout_add(struct ui_layout *, ui_widget *);

struct ui_layout ui_layout_inner(struct ui_layout *);
struct ui_layout ui_layout_split_x(struct ui_layout *, unit w);
struct ui_layout ui_layout_split_y(struct ui_layout *, unit h);

void ui_layout_sep_x(struct ui_layout *, unit px);
void ui_layout_sep_col(struct ui_layout *);
void ui_layout_sep_cols(struct ui_layout *, size_t n);
void ui_layout_tab(struct ui_layout *, size_t n);
void ui_layout_mid(struct ui_layout *, unit width);

void ui_layout_sep_y(struct ui_layout *, unit px);
void ui_layout_sep_row(struct ui_layout *);
void ui_layout_next_row(struct ui_layout *);

void ui_layout_dir(struct ui_layout *, enum ui_layout_dir);
void ui_layout_dir_hori(struct ui_layout *, enum ui_layout_dir);
void ui_layout_dir_vert(struct ui_layout *, enum ui_layout_dir);

inline bool ui_layout_is_nil(struct ui_layout *layout)
{
    return point_is_nil(layout->base.pos) && dim_is_nil(layout->base.dim);
}
