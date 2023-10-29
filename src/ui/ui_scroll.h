/* ui_scroll.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct ui_scroll_style
{
    struct rgba fg, bg;
    unit width;
};

void ui_scroll_style_default(struct ui_style *);

enum ui_scroll_drag { ui_scroll_nil = 0, ui_scroll_rows, ui_scroll_cols, };

struct ui_scroll
{
    ui_widget w;
    struct ui_scroll_style s;

    struct dim cell;
    struct { bool show; size_t first, total, visible; } rows, cols;
    struct { enum ui_scroll_drag type; unit start, bar; } drag;
    struct rowcol center;
};

struct ui_scroll ui_scroll_new(struct dim dim, struct dim cell);
void ui_scroll_free(struct ui_scroll *);

void ui_scroll_update_rows(struct ui_scroll *, size_t total);
void ui_scroll_update_cols(struct ui_scroll *, size_t total);

void ui_scroll_move_rows(struct ui_scroll *, ssize_t inc);
void ui_scroll_move_cols(struct ui_scroll *, ssize_t inc);

void ui_scroll_page_up(struct ui_scroll *);
void ui_scroll_page_down(struct ui_scroll *);

size_t ui_scroll_first_row(const struct ui_scroll *);
size_t ui_scroll_last_row(const struct ui_scroll *);
size_t ui_scroll_first_col(const struct ui_scroll *);
size_t ui_scroll_last_col(const struct ui_scroll *);

void ui_scroll_visible(struct ui_scroll *, size_t row, size_t col);
void ui_scroll_center(struct ui_scroll *, size_t row, size_t col);

void ui_scroll_event(struct ui_scroll *);
struct ui_layout ui_scroll_render(struct ui_scroll *, struct ui_layout *);



