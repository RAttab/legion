/* ui_doc.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct lisp;


// -----------------------------------------------------------------------------
// doc
// -----------------------------------------------------------------------------

struct ui_doc_style
{
    struct
    {
        enum render_font font;
        struct rgba fg, bg;
    } text, bold, link, hover, pressed;

    struct {
        enum render_font font;
        struct rgba fg, bg;
        struct rgba comment, keyword, atom;
    } code;

    struct { struct rgba fg; int8_t offset; } underline;

    struct {
        unit margin;
        enum render_font font;
        struct rgba fg, bg, hover, pressed, border;
    } copy;
};

void ui_doc_style_default(struct ui_style *);


struct ui_doc
{
    ui_widget w;
    struct ui_doc_style s;
    struct ui_panel *p;

    struct ui_scroll scroll;
    uint32_t cols;

    man_page page;
    struct man *man;

    struct {
        man_line line; struct rect rect;
        char *buffer; size_t len, cap;
    } copy;
};

struct ui_doc ui_doc_new(struct dim);
void ui_doc_free(struct ui_doc *);

void ui_doc_open(struct ui_doc *, struct link, struct lisp *);

void ui_doc_event(struct ui_doc *);
void ui_doc_render(struct ui_doc *, struct ui_layout *);
