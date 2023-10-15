/* asm.h
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct mod;
struct assembly;

// -----------------------------------------------------------------------------
// asm
// -----------------------------------------------------------------------------

struct ui_asm_style
{
    enum render_font font;
    struct { unit margin; } find;
    struct { struct rgba current, base; } jmp;
    struct { struct rgba fg; time_sys blink; } carret;
    struct { struct rgba fg, bg, hover; } row, bp;
    struct { struct rgba bg; time_sys opaque, fade; } hl;
    struct rgba fg, keyword, symbol;
    struct rgba current, select, highlight;
};

void ui_asm_style_default(struct ui_style *);

enum ui_asm_find_type : uint8_t
{
    ui_asm_find_nil = 0,
    ui_asm_find_row,
    ui_asm_find_text,
};

struct ui_asm
{
    ui_widget w;
    struct ui_asm_style s;
    struct ui_panel *p;

    struct ui_scroll scroll;
    struct rect inner;

    struct assembly *as;
    const struct mod *mod;

    struct rowcol carret;
    struct { vm_ip ip; uint32_t row; } bp;
    struct { uint32_t row; time_sys ts; } hl;
    struct { bool active; struct rowcol first, last; } select;

    struct {
        enum ui_asm_find_type type;
        struct ui_label op;
        struct ui_input value;
        struct ui_button exec, close;
        size_t len;
    } find;
};

struct ui_asm ui_asm_new(struct dim);
void ui_asm_free(struct ui_asm *);

void ui_asm_reset(struct ui_asm *);
void ui_asm_set_mod(struct ui_asm *, const struct mod *);

void ui_asm_focus(struct ui_asm *);

vm_ip ui_asm_ip(struct ui_asm *);
void ui_asm_goto(struct ui_asm *, vm_ip);
void ui_asm_breakpoint(struct ui_asm *, vm_ip);

void ui_asm_event(struct ui_asm *);
void ui_asm_render(struct ui_asm *, struct ui_layout *);
