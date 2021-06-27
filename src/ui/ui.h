/* gui.h
   Rémi Attab (remi.attab@gmail.com), 10 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/log.h"
#include "utils/text.h"
#include "vm/mod.h"
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

struct ui_widget
{
    struct pos pos;
    struct dim dim;
};

inline struct ui_widget ui_widget_new(int16_t w, int16_t h)
{
    return (struct ui_widget) {
        .dim = (struct dim) { .w = w, .h = h },
    };
}

inline struct SDL_Rect ui_widget_rect(const struct ui_widget *widget)
{
    return (SDL_Rect) {
        .x = widget->pos.x, .y = widget->pos.y,
        .w = widget->dim.w, .h = widget->dim.h,
    };
}


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct ui_layout
{
    struct pos top;
    struct dim dim;
    struct dim pad;

    struct pos pos;
    int16_t next_y;
};

enum { ui_layout_inf = -1 };

struct ui_layout ui_layout_new(struct pos, struct dim);
void ui_layout_add(struct ui_layout *, struct ui_widget *);

void ui_layout_next_row(struct ui_layout *);
void ui_layout_sep_x(struct ui_layout *, int16_t px);
void ui_layout_sep_y(struct ui_layout *, int16_t px);
void ui_layout_mid(struct ui_layout *, const struct ui_widget *);
void ui_layout_right(struct ui_layout *, const struct ui_widget *);

inline bool ui_layout_is_nil(struct ui_layout *layout)
{
    return pos_is_nil(layout->top) && dim_is_nil(layout->dim);
}


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

struct ui_str
{
    uint8_t len, cap;
    const char *str;
};

enum { ui_str_cap = 128 };

struct ui_str ui_str_c(const char *);
struct ui_str ui_str_v(size_t len);
struct ui_str ui_str_clone(const struct ui_str *);
void ui_str_free(struct ui_str *);
void ui_str_setc(struct ui_str *, const char *str);
void ui_str_setv(struct ui_str *, const char *str, size_t len);
void ui_str_setf(struct ui_str *, const char *fmt, ...) legion_printf(2, 3);
void ui_str_set_u64(struct ui_str *, uint64_t val);
void ui_str_set_hex(struct ui_str *, uint64_t val);
void ui_str_set_scaled(struct ui_str *, uint64_t val);
void ui_str_set_id(struct ui_str *, id_t val);
inline size_t ui_str_len(struct ui_str *str) { return str->cap ? str->cap : str->len; }


// -----------------------------------------------------------------------------
// label
// -----------------------------------------------------------------------------

struct ui_label
{
    struct ui_widget w;
    struct ui_str str;

    struct font *font;
    struct rgba fg, bg;
};

struct ui_label ui_label_new(struct font *, struct ui_str);
void ui_label_free(struct ui_label *);
void ui_label_render(struct ui_label *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

enum ui_button_state
{
    ui_button_idle = 0,
    ui_button_hover,
    ui_button_pressed,
};

struct ui_button
{
    struct ui_widget w;
    struct ui_str str;

    struct font *font;
    struct dim pad;

    bool disabled;
    enum ui_button_state state;
};

struct ui_button ui_button_new(struct font *, struct ui_str);
void ui_button_free(struct ui_button *);
enum ui_ret ui_button_event(struct ui_button *, const SDL_Event *);
void ui_button_render(struct ui_button *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct ui_scroll
{
    struct ui_widget w;

    int16_t row_h;
    size_t first, total, visible;
    struct { int16_t start, bar; } drag;
};

struct ui_scroll ui_scroll_new(struct dim dim, int16_t row_h);
void ui_scroll_free(struct ui_scroll *);

void ui_scroll_move(struct ui_scroll *, ssize_t inc);
void ui_scroll_update(struct ui_scroll *, size_t total);

enum ui_ret ui_scroll_event(struct ui_scroll *, const SDL_Event *);
struct ui_layout ui_scroll_render(struct ui_scroll *, struct ui_layout *, SDL_Renderer *);

inline size_t ui_scroll_first(const struct ui_scroll *scroll) { return scroll->first; }
inline size_t ui_scroll_last(const struct ui_scroll *scroll)
{
    return legion_min(scroll->total, scroll->first + scroll->visible);
}


// -----------------------------------------------------------------------------
// toggle
// -----------------------------------------------------------------------------

enum ui_toggle_state
{
    ui_toggle_idle = 0,
    ui_toggle_hover,
    ui_toggle_selected,
};

struct ui_toggle
{
    struct ui_widget w;
    struct ui_str str;

    struct font *font;
    uint64_t user;

    enum ui_toggle_state state;
};

struct ui_toggle ui_toggle_new(struct font *, struct ui_str);
void ui_toggle_free(struct ui_toggle *);
enum ui_ret ui_toggle_event(struct ui_toggle *, const SDL_Event *);
void ui_toggle_render(struct ui_toggle *, struct ui_layout *, SDL_Renderer *);


struct ui_toggles
{
    struct font *font;
    struct ui_str str;

    size_t len, cap;
    struct ui_toggle *items;
};

struct ui_toggles ui_toggles_new(struct font *, struct ui_str);
void ui_toggles_free(struct ui_toggles *);
void ui_toggles_resize(struct ui_toggles *, size_t len);

void ui_toggles_clear(struct ui_toggles *);
void ui_toggles_select(struct ui_toggles *, uint64_t user);

enum ui_ret ui_toggles_event(
        struct ui_toggles *, const SDL_Event *, const struct ui_scroll *,
        struct ui_toggle **r_toggle, size_t *r_index);
void ui_toggles_render(
        struct ui_toggles *, struct ui_layout *, SDL_Renderer *, const struct ui_scroll *);


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

struct ui_input
{
    struct ui_widget w;

    struct font *font;

    bool focused;
    struct { char *c; uint8_t len, cap; } buf;
    struct { uint8_t col; bool blink; } carret;
};

struct ui_input ui_input_new(struct font *, size_t cap);
void ui_input_free(struct ui_input *);

void ui_input_clear(struct ui_input *);
void ui_input_set(struct ui_input *, const char *str);

uint64_t ui_input_get_u64(struct ui_input *);
uint64_t ui_input_get_hex(struct ui_input *);
size_t ui_input_get_atom(struct ui_input *, atom_t *dst);

enum ui_ret ui_input_event(struct ui_input *, const SDL_Event *);
void ui_input_render(struct ui_input *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code
{
    struct ui_widget w;
    struct ui_scroll scroll;
    struct ui_label num, code;

    struct font *font;
    bool focused;

    struct text text;
    struct mod *mod;

    struct {
        bool blink;
        size_t row, col;
        struct line *line;
    } carret;
};

enum { ui_code_num_len = 4 };

struct ui_code ui_code_new(struct dim, struct font *);
void ui_code_free(struct ui_code *);

void ui_code_clear(struct ui_code *);
void ui_code_set(struct ui_code *, struct mod *, ip_t);

enum ui_ret ui_code_event(struct ui_code *, const SDL_Event *);
void ui_code_render(struct ui_code *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

enum ui_panel_state
{
    ui_panel_hidden = 0,
    ui_panel_visible = 1,
    ui_panel_focused = 2,
};

struct ui_panel
{
    struct ui_widget w;
    struct ui_layout layout;

    struct ui_label title;
    struct ui_button close;

    bool menu;
    enum ui_panel_state state;
};

struct ui_panel ui_panel_menu(struct pos, struct dim);
struct ui_panel ui_panel_title(struct pos, struct dim, struct ui_str);
void ui_panel_free(struct ui_panel *);
enum ui_ret ui_panel_event(struct ui_panel *, const SDL_Event *);
struct ui_layout ui_panel_render(struct ui_panel *, SDL_Renderer *);
