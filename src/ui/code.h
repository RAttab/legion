/* code.h
   Rémi Attab (remi.attab@gmail.com), 16 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "vm/vm.h"
#include "ui/scroll.h"
#include "ui/tooltip.h"
#include "ui/list.h"
#include "utils/time.h"

struct mod;
struct code;

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct ui_code_style
{
    const struct font *font;
    struct { struct rgba fg, bg; } row, bp;
    struct { struct rgba fg; time_sys blink; } carret;
    struct { struct rgba bg; time_sys opaque, fade; } hl;
    struct { struct rgba fg, bg; } errors;
    struct rgba fg, comment, keyword, atom;
    struct rgba current, select, match, box;
};

void ui_code_style_default(struct ui_style *);

struct ui_code
{
    struct ui_widget w;
    struct ui_code_style s;
    struct ui_panel *p;

    bool focused, writable, modified;
    SDL_Rect inner;
    struct ui_scroll scroll;
    struct ui_tooltip tooltip;
    struct ui_list errors;

    struct code *code;
    const struct mod *mod;

    time_sys edit;
    struct { uint32_t pos, row, col; } carret;
    struct { uint32_t pos, row, col; vm_ip ip; } bp;
    struct { uint32_t len, row, col; time_sys ts; } hl;
    struct { hash_val sym; uint32_t paren; } match;

    struct
    {
        bool active;
        struct { uint32_t pos, row, col; } first, last;
    } select;
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

enum ui_ret ui_code_event(struct ui_code *, const SDL_Event *);
void ui_code_render(struct ui_code *, struct ui_layout *, SDL_Renderer *);
