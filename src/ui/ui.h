/* gui.h
   Rémi Attab (remi.attab@gmail.com), 10 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/log.h"

#include "SDL.h"

struct font;


// -----------------------------------------------------------------------------
// ret
// -----------------------------------------------------------------------------

enum ui_ret
{
    ui_nil = 0,
    ui_skip = 1 << 0,
    ui_action = 1 << 1,
    ui_consume = 1 << 2,
};


// -----------------------------------------------------------------------------
// pos
// -----------------------------------------------------------------------------

struct pos { int16_t x, y; };

inline struct pos make_pos(int16_t x, int16_t y)
{
    return (struct pos) { .x = x, .y = y };
}

inline struct pos make_pos_from_rect(SDL_Rect rect)
{
    return make_pos(rect.x, rect.y);
}

inline SDL_Point pos_as_point(struct pos pos)
{
    return (SDL_Point) { .x = pos.x, .y = pos.y };
}

inline bool pos_is_nil(struct pos pos) { return !pos.x && !pos.y; }


// -----------------------------------------------------------------------------
// dim
// -----------------------------------------------------------------------------

struct dim { int16_t w, h; };

inline struct dim make_dim(int16_t w, int16_t h)
{
    return (struct dim) { .w = w, .h = h };
}

inline struct dim make_dim_from_rect(SDL_Rect rect)
{
    return make_dim(rect.w, rect.h);
}

inline bool dim_is_nil(struct dim dim) { return !dim.w && !dim.h; }


// -----------------------------------------------------------------------------
// rgba
// -----------------------------------------------------------------------------

struct rgba { uint8_t r, g, b, a; };
inline struct rgba make_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (struct rgba) { .r = r, .g = g, .b = b, .a = a };
}

inline struct rgba rgba_nil(void)       { return make_rgba(0x00, 0x00, 0x00, 0x00); }
inline struct rgba rgba_gray_a(uint8_t v, uint8_t a) { return make_rgba(v, v, v, a); }
inline struct rgba rgba_gray(uint8_t v) { return rgba_gray_a(v, 0xFF); }
inline struct rgba rgba_white(void )    { return rgba_gray(0xFF); }
inline struct rgba rgba_red(void)       { return make_rgba(0xCC, 0x00, 0x00, 0xFF); }
inline struct rgba rgba_green(void)     { return make_rgba(0x00, 0xCC, 0x00, 0xFF); }
inline struct rgba rgba_blue(void)      { return make_rgba(0x00, 0x00, 0xCC, 0xFF); }

inline void rgba_render(struct rgba c, SDL_Renderer *renderer)
{
    sdl_err(SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a));
    sdl_err(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND));
};


// -----------------------------------------------------------------------------
// widget
// -----------------------------------------------------------------------------

struct widget
{
    struct pos pos;
    struct dim dim;
};

inline struct widget make_widget(int16_t w, int16_t h)
{
    return (struct widget) {
        .dim = (struct dim) { .w = w, .h = h },
    };
}

inline struct SDL_Rect widget_rect(const struct widget *widget)
{
    return (SDL_Rect) {
        .x = widget->pos.x, .y = widget->pos.y,
        .w = widget->dim.w, .h = widget->dim.h,
    };
}


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct layout
{
    struct pos top;
    struct dim dim;
    struct dim pad;

    struct pos pos;
    int16_t next_y;
};

enum { layout_inf = -1 };

struct layout layout_new(struct pos, struct dim);
void layout_add(struct layout *, struct widget *);

void layout_next_row(struct layout *);
void layout_sep_x(struct layout *, int16_t px);
void layout_sep_y(struct layout *, int16_t px);
void layout_mid(struct layout *, const struct widget *);
void layout_right(struct layout *, const struct widget *);

inline bool layout_is_nil(struct layout *layout)
{
    return pos_is_nil(layout->top) && dim_is_nil(layout->dim);
}


// -----------------------------------------------------------------------------
// label
// -----------------------------------------------------------------------------

struct label
{
    struct widget w;

    struct font *font;
    struct rgba fg, bg;

    uint8_t len;
    const char *str;
};

enum { label_cap = 128 };

struct label *label_const(struct font *, const char *str);
struct label *label_var(struct font *, size_t len);
void label_set(struct label *, const char *str, size_t len);

void label_render(struct label *, struct layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

enum button_state
{
    button_idle = 0,
    button_hover,
    button_pressed,
};

struct button
{
    struct widget w;

    struct font *font;
    struct rgba fg;
    struct dim pad;

    enum button_state state;

    uint8_t len;
    const char *str;
};

enum { button_cap = 128 };

struct button *button_const(struct font *, const char *str);
struct button *button_var(struct font *, size_t len);
void button_set(struct button *, const char *str, size_t len);

enum ui_ret button_event(struct button *, const SDL_Event *);
void button_render(struct button *, struct layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// toggle
// -----------------------------------------------------------------------------

enum toggle_state
{
    toggle_idle = 0,
    toggle_hover,
    toggle_selected,
};

struct toggle
{
    struct widget w;

    struct font *font;
    struct rgba fg;

    enum toggle_state state;

    uint8_t len;
    const char *str;
};

enum { toggle_cap = 128 };

struct toggle *toggle_const(struct font *, const char *str);
struct toggle *toggle_var(struct font *, size_t len);
void toggle_set(struct toggle *, const char *str, size_t len);

enum ui_ret toggle_event(struct toggle *, const SDL_Event *);
void toggle_render(struct toggle *, struct layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct scroll
{
    struct widget w;

    size_t first, total, visible;
    struct { int16_t y, top; } drag;
};

struct scroll *scroll_new(struct dim dim, size_t total, size_t visible);
void scroll_move(struct scroll *, ssize_t inc);
void scroll_update(struct scroll *, size_t total);

enum ui_ret scroll_event(struct scroll *, const SDL_Event *);
struct layout scroll_render(struct scroll *, struct layout *, SDL_Renderer *);

inline size_t scroll_first(const struct scroll *scroll) { return scroll->first; }
inline size_t scroll_last(const struct scroll *scroll)
{
    return legion_min(scroll->total, scroll->first + scroll->visible);
}


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

enum panel_state
{
    panel_hidden = 0,
    panel_visible = 1,
    panel_focused = 2,
};

struct panel
{
    struct widget w;
    struct layout layout;

    enum panel_state state;

    struct label *title;
    struct button *close;
};

struct panel *panel_slim(struct pos, struct dim);
struct panel *panel_const(struct pos, struct dim, const char *str);
struct panel *panel_var(struct pos, struct dim, size_t len);

enum ui_ret panel_event(struct panel *, const SDL_Event *);
struct layout panel_render(struct panel *, SDL_Renderer *);
