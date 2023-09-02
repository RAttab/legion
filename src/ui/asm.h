/* asm.h
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "vm/vm.h"
#include "ui/scroll.h"
#include "utils/str.h"

struct mod;
struct assembly;

// -----------------------------------------------------------------------------
// asm
// -----------------------------------------------------------------------------

struct ui_asm_style
{
    const struct font *font;
    struct { struct rgba fg, bg; } row, bp;
    struct { struct rgba current, base; } jmp;
    struct { struct rgba fg; time_sys blink; } carret;
    struct { struct rgba bg; time_sys opaque, fade; } hl;
    struct rgba fg, keyword, symbol;
    struct rgba current, highlight;
};

void ui_asm_style_default(struct ui_style *);

struct ui_asm
{
    struct ui_widget w;
    struct ui_asm_style s;
    struct ui_panel *p;

    bool focused;
    struct ui_scroll scroll;

    struct assembly *as;
    const struct mod *mod;

    struct rowcol carret;
    struct { vm_ip ip; uint32_t row; } bp;
    struct { uint32_t row; time_sys ts; } hl;
};

struct ui_asm ui_asm_new(struct dim);
void ui_asm_free(struct ui_asm *);

void ui_asm_reset(struct ui_asm *);
void ui_asm_set_mod(struct ui_asm *, const struct mod *);

void ui_asm_focus(struct ui_asm *);

vm_ip ui_asm_ip(struct ui_asm *);
void ui_asm_goto(struct ui_asm *, vm_ip);
void ui_asm_breakpoint(struct ui_asm *, vm_ip);

enum ui_ret ui_asm_event(struct ui_asm *, const SDL_Event *);
void ui_asm_render(struct ui_asm *, struct ui_layout *, SDL_Renderer *);
