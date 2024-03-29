/* ui_code.h
   Rémi Attab (remi.attab@gmail.com), 16 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


struct mod;
struct code;

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code_style
{
    enum render_font font, match;
    struct { unit margin; } find;
    struct { struct rgba fg, bg; } row;
    struct { struct rgba fg, bg, hover; } bp;
    struct { struct rgba fg; sys_ts blink; } carret;
    struct { struct rgba bg; sys_ts opaque, fade; } hl;
    struct { struct rgba fg, bg; unit margin; } errors;
    struct rgba fg, comment, keyword, atom;
    struct rgba current, select, box;
};

void ui_code_style_default(struct ui_style *);

enum ui_code_find_type : uint8_t
{
    ui_code_find_nil = 0,
    ui_code_find_row,
    ui_code_find_text,
    ui_code_find_replace,
};

struct ui_code
{
    ui_widget w;
    struct ui_code_style s;
    struct ui_panel *p;

    bool writable, modified;
    struct rect margin, inner;
    struct ui_scroll scroll;
    struct ui_list errors;

    struct code *code;
    const struct mod *mod;

    sys_ts edit;
    struct { uint32_t pos, row, col; } carret;
    struct { uint32_t pos, row, col; vm_ip ip; } bp;
    struct { uint32_t len, row, col; sys_ts ts; } hl;
    struct { hash_val sym; uint32_t paren; } match;

    struct
    {
        bool active;
        struct { uint32_t pos, row, col; } first, last;
    } select;

    struct
    {
        enum ui_code_find_type type;
        struct ui_label op, by;
        struct ui_input value, replace;
        struct ui_button exec, close;
        uint32_t len;
    } find;
};

struct ui_code ui_code_new(struct dim);
void ui_code_free(struct ui_code *);

void ui_code_reset(struct ui_code *);
void ui_code_set_mod(struct ui_code *, const struct mod *);
void ui_code_set_text(struct ui_code *, const char *, size_t len);

void ui_code_focus(struct ui_code *);
bool ui_code_modified(struct ui_code *);

vm_ip ui_code_ip(struct ui_code *);
void ui_code_goto(struct ui_code *, vm_ip);
void ui_code_breakpoint(struct ui_code *, vm_ip);

void ui_code_event(struct ui_code *);
void ui_code_render(struct ui_code *, struct ui_layout *);
