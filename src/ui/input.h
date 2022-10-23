/* input.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "vm/vm.h"

struct symbol;


// -----------------------------------------------------------------------------
// input
// -----------------------------------------------------------------------------

enum { ui_input_cap = 256 };

struct ui_input_style
{
    const struct font *font;
    struct rgba fg, bg, border, carret;
    struct dim pad;
};

struct ui_input
{
    struct ui_widget w;
    struct ui_input_style s;
    struct ui_panel *p;

    bool focused;
    struct { uint8_t col, len; } view;
    struct { char *c; uint8_t len; } buf;
    struct { uint8_t col; bool blink; } carret;
};

struct ui_input ui_input_new(size_t len);
void ui_input_free(struct ui_input *);

void ui_input_focus(struct ui_input *);

void ui_input_clear(struct ui_input *);
void ui_input_set(struct ui_input *, const char *str);

bool ui_input_get_u64(struct ui_input *, uint64_t *ret);
bool ui_input_get_symbol(struct ui_input *, struct symbol *ret);
bool ui_input_eval(struct ui_input *, vm_word *ret);

enum ui_ret ui_input_event(struct ui_input *, const SDL_Event *);
void ui_input_render(struct ui_input *, struct ui_layout *, SDL_Renderer *);
