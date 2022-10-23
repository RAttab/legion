/* code.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "vm/vm.h"
#include "utils/text.h"

#include "types.h"
#include "scroll.h"
#include "tooltip.h"

struct mod;


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

enum { ui_code_num_len = 4 };


struct ui_code_style
{
    const struct font *font;
    struct { struct rgba fg, bg; } line, code;
    struct { struct rgba fg, bg, hover; uint8_t margin; } breakpoint;
    struct rgba mark, error, carret;
};

void ui_code_style_default(struct ui_style *);


struct ui_code
{
    struct ui_widget w;
    struct ui_code_style s;
    struct ui_panel *p;

    struct ui_scroll scroll;
    struct ui_tooltip tooltip;

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
    struct { vm_ip ip; size_t row; } breakpoint;
};

struct ui_code ui_code_new(struct dim);
void ui_code_free(struct ui_code *);

void ui_code_focus(struct ui_code *);

void ui_code_clear(struct ui_code *);
void ui_code_set_code(struct ui_code *, const struct mod *, vm_ip);
void ui_code_set_disassembly(struct ui_code *, const struct mod *, vm_ip);
void ui_code_set_text(struct ui_code *code, const char *text, size_t len);

vm_ip ui_code_ip(struct ui_code *);
void ui_code_goto(struct ui_code *, vm_ip);
void ui_code_indent(struct ui_code *);
void ui_code_breakpoint(struct ui_code *, size_t row);
void ui_code_breakpoint_ip(struct ui_code *, vm_ip ip);

enum ui_ret ui_code_event(struct ui_code *, const SDL_Event *);
void ui_code_render(struct ui_code *, struct ui_layout *, SDL_Renderer *);
