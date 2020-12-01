/* text.h
   Rémi Attab (remi.attab@gmail.com), 25 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// line
// -----------------------------------------------------------------------------

enum { line_cap = 64 - (8 * 2) - 1 };

struct legion_packed line
{
    struct line *next, *prev;
    char c[line_cap];
    char zero;
};

bool line_empty(struct line *);
size_t line_len(struct line *);

void line_put(struct line *, size_t index, char c);
void line_del(struct line *, size_t index);


// -----------------------------------------------------------------------------
// text
// -----------------------------------------------------------------------------

struct text
{
    size_t len;
    struct line *first, *last;
};

void text_init(struct text *);
void text_clear(struct text *);

struct line *text_goto(struct text *, size_t line);
struct line *text_insert(struct text *, struct line *at);
struct line *text_erase(struct text *, struct line *at);

void text_pack(const struct text *, char *dst, size_t len);
void text_unpack(struct text *, const char *src, size_t len);
