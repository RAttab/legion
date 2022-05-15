/* gui.h
   Rémi Attab (remi.attab@gmail.com), 10 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "items/item.h"
#include "game/coord.h"
#include "game/man.h"
#include "utils/text.h"
#include "utils/color.h"
#include "vm/mod.h"
#include "SDL.h"

struct font;
struct hset;
struct lisp;


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
void ui_layout_add(struct ui_layout *, struct ui_widget *);
struct ui_layout ui_layout_inner(struct ui_layout *);

void ui_layout_next_row(struct ui_layout *);
void ui_layout_sep_x(struct ui_layout *, int16_t px);
void ui_layout_sep_y(struct ui_layout *, int16_t px);
void ui_layout_mid(struct ui_layout *, int width);
void ui_layout_dir(struct ui_layout *, enum ui_layout_dir);

inline bool ui_layout_is_nil(struct ui_layout *layout)
{
    return pos_is_nil(layout->base.pos) && dim_is_nil(layout->base.dim);
}


// -----------------------------------------------------------------------------
// clipboard
// -----------------------------------------------------------------------------

struct ui_clipboard
{
    size_t len, cap;
    char *str;
};

void ui_clipboard_init(struct ui_clipboard *);
void ui_clipboard_free(struct ui_clipboard *);

size_t ui_clipboard_paste(struct ui_clipboard *, size_t len, char *dst);

void ui_clipboard_copy(struct ui_clipboard *, size_t len, const char *src);
void ui_clipboard_copy_hex(struct ui_clipboard *, uint64_t val);


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
struct ui_str ui_str_c(const char *);
struct ui_str ui_str_v(size_t len);
struct ui_str ui_str_clone(const struct ui_str *);
void ui_str_free(struct ui_str *);

void ui_str_copy(struct ui_str *, struct ui_clipboard *);
void ui_str_paste(struct ui_str *, struct ui_clipboard *);

void ui_str_setc(struct ui_str *, const char *str);
void ui_str_setv(struct ui_str *, const char *str, size_t len);
void ui_str_setvf(struct ui_str *, const char *fmt, va_list);
void ui_str_setf(struct ui_str *, const char *fmt, ...) legion_printf(2, 3);
void ui_str_set_u64(struct ui_str *, uint64_t val);
void ui_str_set_hex(struct ui_str *, uint64_t val);
void ui_str_set_scaled(struct ui_str *, uint64_t val);
void ui_str_set_id(struct ui_str *, id_t val);
void ui_str_set_item(struct ui_str *, enum item val);
void ui_str_set_coord(struct ui_str *, struct coord val);
void ui_str_set_symbol(struct ui_str *, const struct symbol *val);
void ui_str_set_atom(struct ui_str *, word_t word);

inline size_t ui_str_len(struct ui_str *str)
{
    return str->cap ? str->cap : str->len;
}


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
// link
// -----------------------------------------------------------------------------

enum ui_link_state
{
    ui_link_idle = 0,
    ui_link_hover,
    ui_link_pressed,
};

struct ui_link
{
    struct ui_widget w;
    struct ui_str str;

    struct font *font;
    struct rgba fg;

    enum ui_link_state state;
};

struct ui_link ui_link_new(struct font *, struct ui_str);
void ui_link_free(struct ui_link *);
enum ui_ret ui_link_event(struct ui_link *, const SDL_Event *);
void ui_link_render(struct ui_link *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip
{
    struct ui_widget w;
    struct ui_str str;

    struct font *font;
    struct rgba fg;
    struct dim pad;

    SDL_Rect rect;
    bool visible;
};

struct ui_tooltip ui_tooltip_new(struct font *, struct ui_str, SDL_Rect);
void ui_tooltip_free(struct ui_tooltip *);
enum ui_ret ui_tooltip_event(struct ui_tooltip *, const SDL_Event *);
void ui_tooltip_render(struct ui_tooltip *, SDL_Renderer *);


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
// input
// -----------------------------------------------------------------------------

enum { ui_input_cap = 256 };

struct ui_input
{
    struct ui_widget w;
    struct font *font;

    bool focused;
    struct { uint8_t col, len; } view;
    struct { char *c; uint8_t len; } buf;
    struct { uint8_t col; bool blink; } carret;
};

struct ui_input ui_input_new(struct font *, size_t char_width);
void ui_input_free(struct ui_input *);

void ui_input_focus(struct ui_input *);

void ui_input_clear(struct ui_input *);
void ui_input_set(struct ui_input *, const char *str);

bool ui_input_get_u64(struct ui_input *, uint64_t *ret);
bool ui_input_get_symbol(struct ui_input *, struct symbol *ret);
bool ui_input_eval(struct ui_input *, word_t *ret);

enum ui_ret ui_input_event(struct ui_input *, const SDL_Event *);
void ui_input_render(struct ui_input *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code
{
    struct ui_widget w;
    struct ui_scroll scroll;
    struct ui_tooltip tooltip;

    struct font *font;
    bool focused;
    size_t cols;

    struct text text;
    const struct mod *mod;
    bool disassembly;

    struct
    {
        bool init;
        size_t top, bot, cols;
        struct line *line;
    } view;

    struct
    {
        bool blink;
        size_t row, col;
        struct line *line;
    } carret;

    struct { size_t row, col, len; } mark;
};

enum { ui_code_num_len = 4 };

struct ui_code ui_code_new(struct dim, struct font *);
void ui_code_free(struct ui_code *);

void ui_code_focus(struct ui_code *);

void ui_code_clear(struct ui_code *);
void ui_code_set_code(struct ui_code *, const struct mod *, ip_t);
void ui_code_set_disassembly(struct ui_code *, const struct mod *, ip_t);
void ui_code_set_text(struct ui_code *code, const char *text, size_t len);

ip_t ui_code_ip(struct ui_code *);
void ui_code_goto(struct ui_code *, ip_t);
void ui_code_indent(struct ui_code *);

enum ui_ret ui_code_event(struct ui_code *, const SDL_Event *);
void ui_code_render(struct ui_code *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// doc
// -----------------------------------------------------------------------------

struct ui_doc
{
    struct ui_widget w;
    struct ui_scroll scroll;

    struct { uint8_t pt, h, w; } font;
    size_t cols;

    page_t page;
    struct man *man;
};

struct ui_doc ui_doc_new(struct dim, int pt);
void ui_doc_free(struct ui_doc *);

void ui_doc_open(struct ui_doc *, struct link, struct lisp *);

enum ui_ret ui_doc_event(struct ui_doc *, const SDL_Event *);
void ui_doc_render(struct ui_doc *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// list
// -----------------------------------------------------------------------------

struct ui_entry
{
    struct ui_str str;
    uint64_t user;
};

struct ui_list
{
    struct ui_widget w;
    struct ui_scroll scroll;

    struct font *font;
    struct ui_str str;

    size_t len, cap;
    struct ui_entry *entries;

    uint64_t hover, selected;
};


struct ui_list ui_list_new(struct dim, struct font *, size_t chars);
void ui_list_free(struct ui_list *);

void ui_list_clear(struct ui_list *);
bool ui_list_select(struct ui_list *, uint64_t user);

void ui_list_reset(struct ui_list *);
struct ui_str *ui_list_add(struct ui_list *, uint64_t user);

enum ui_ret ui_list_event(struct ui_list *, const SDL_Event *);
void ui_list_render(struct ui_list *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// tree
// -----------------------------------------------------------------------------

typedef uint32_t ui_node_t;
static legion_unused ui_node_t ui_node_nil = -1;

struct ui_node
{
    bool open, leaf;
    uint8_t depth;
    struct ui_str str;
    uint64_t user;
    ui_node_t parent;
};

struct ui_tree
{
    struct ui_widget w;
    struct ui_scroll scroll;

    struct font *font;
    struct ui_str str;

    ui_node_t len, cap;
    struct ui_node *nodes;

    uint64_t hover, selected;
    struct hset *open;
};


struct ui_tree ui_tree_new(struct dim, struct font *, size_t chars);
void ui_tree_free(struct ui_tree *);

struct ui_node *ui_tree_node(struct ui_tree *, ui_node_t);
ui_node_t ui_tree_user(struct ui_tree *, uint64_t user);

void ui_tree_clear(struct ui_tree *);
ui_node_t ui_tree_select(struct ui_tree *, uint64_t user);

void ui_tree_reset(struct ui_tree *);
ui_node_t ui_tree_index(struct ui_tree *);
struct ui_str *ui_tree_add(struct ui_tree *, ui_node_t parent, uint64_t user);

enum ui_ret ui_tree_event(struct ui_tree *, const SDL_Event *);
void ui_tree_render(struct ui_tree *, struct ui_layout *, SDL_Renderer *);


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

void ui_panel_resize(struct ui_panel *, struct dim);
void ui_panel_show(struct ui_panel *);
void ui_panel_hide(struct ui_panel *);
bool ui_panel_is_visible(struct ui_panel *);

enum ui_ret ui_panel_event(struct ui_panel *, const SDL_Event *);
enum ui_ret ui_panel_event_consume(struct ui_panel *, const SDL_Event *);
struct ui_layout ui_panel_render(struct ui_panel *, SDL_Renderer *);
