/* doc.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "scroll.h"
#include "game/man.h"


// -----------------------------------------------------------------------------
// doc
// -----------------------------------------------------------------------------

struct ui_doc_style
{
    struct
    {
        const struct font *font;
        struct rgba fg, bg;
    } text, bold, link, hover, pressed;

    struct {
        const struct font *font;
        struct rgba fg, bg;
        struct rgba comment, keyword, atom;
    } code;

    struct { struct rgba fg; int8_t offset; } underline;

    struct {
        int16_t margin;
        const struct font *font;
        struct rgba fg, bg, hover, pressed, border;
    } copy;
};

void ui_doc_style_default(struct ui_style *);


struct ui_doc
{
    struct ui_widget w;
    struct ui_doc_style s;

    struct ui_scroll scroll;
    bool pressed;
    size_t cols;

    man_page page;
    struct man *man;

    struct {
        man_line line; SDL_Rect rect;
        char *buffer; size_t len, cap;
    } copy;
};

struct ui_doc ui_doc_new(struct dim);
void ui_doc_free(struct ui_doc *);

void ui_doc_open(struct ui_doc *, struct link, struct lisp *);

enum ui_ret ui_doc_event(struct ui_doc *, const SDL_Event *);
void ui_doc_render(struct ui_doc *, struct ui_layout *, SDL_Renderer *);
