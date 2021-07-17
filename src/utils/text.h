/* text.h
   Rémi Attab (remi.attab@gmail.com), 25 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


struct text;

// -----------------------------------------------------------------------------
// line
// -----------------------------------------------------------------------------

struct legion_packed line
{
    struct line *next, *prev;
    uint16_t len, cap;
    char c[];
};

struct line_ret
{
    struct line *line;
    uint16_t index;
};

struct line_ret line_insert(struct text *, struct line *, size_t index, char c);
struct line_ret line_delete(struct text *, struct line *, size_t index);
struct line_ret line_backspace(struct text *, struct line *, size_t index);


// -----------------------------------------------------------------------------
// text
// -----------------------------------------------------------------------------

struct text
{
    size_t lines, bytes;
    struct line *first, *last;
};

void text_init(struct text *);
void text_clear(struct text *);

struct line *text_goto(struct text *, size_t line);
struct line *text_insert(struct text *, struct line *at);
struct line *text_erase(struct text *, struct line *at);

void text_indent(struct text *);
size_t text_indent_at(struct text *, struct line *);

void text_to_str(const struct text *, char *dst, size_t len);
void text_from_str(struct text *, const char *src, size_t len);
