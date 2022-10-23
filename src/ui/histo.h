/* histo.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "label.h"
#include "input.h"
#include "button.h"


// -----------------------------------------------------------------------------
// histo
// -----------------------------------------------------------------------------

typedef uint64_t ui_histo_data;

struct ui_histo_style
{
    struct dim pad;
    struct rgba edge, border;
    struct { int16_t h, pad; } row;
    struct { struct rgba fg, bg; } hover;
    struct { struct dim pad; struct rgba fg; } axes;
    struct { const struct font *font; struct rgba fg, bg; } value;
};

void ui_histo_style_default(struct ui_style *);


struct ui_histo_series
{
    uint8_t col;
    const char *name;
    struct rgba fg;
    bool visible;
};

struct ui_histo_legend
{
    struct ui_label name, val, tick;
};


struct ui_histo
{
    struct ui_widget w;
    struct ui_histo_style s;

    struct pos inner;
    struct dim row;
    int16_t legend_h;

    struct { ui_histo_data bound; } v;
    struct { ui_histo_data scale; } t;
    struct { ui_histo_data t; size_t row; } edge;
    struct { ui_histo_data t, v; size_t row; bool active; } hover;
    struct {
        size_t len, cols, rows;
        ui_histo_data *data;
        struct ui_histo_series *list;
    } series;

    struct {
        size_t rows;
        struct ui_histo_legend *list;
        struct ui_label time, time_val, per_tick;
        struct {
            struct ui_label label;
            struct ui_input val;
            struct ui_button set;
        } scale;
    } legend;
};

struct ui_histo ui_histo_new(
        struct dim, const struct ui_histo_series *, size_t len);
void ui_histo_free(struct ui_histo *);

void ui_histo_clear(struct ui_histo *);

ui_histo_data *ui_histo_at(struct ui_histo *, size_t row);
ui_histo_data ui_histo_row_t(struct ui_histo *, size_t row);
struct ui_histo_series *ui_histo_series(struct ui_histo *, size_t i);

void ui_histo_scale_t(struct ui_histo *, ui_histo_data scale);
void ui_histo_advance(struct ui_histo *, ui_histo_data t);
void ui_histo_push(struct ui_histo *, size_t series, ui_histo_data v);

void ui_histo_update_legend(struct ui_histo *);

enum ui_ret ui_histo_event(struct ui_histo *, const SDL_Event *);
void ui_histo_render(struct ui_histo *, struct ui_layout *, SDL_Renderer *);
