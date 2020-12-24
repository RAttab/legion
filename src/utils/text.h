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

enum { text_line_cap = s_cache_line - (2*sizeof(struct line *)) - 1 };

struct legion_packed line
{
    struct line *next, *prev;
    char c[text_line_cap];
    char zero;
};

static_assert(sizeof(struct line) == s_cache_line);

bool line_empty(struct line *);
size_t line_len(struct line *);

struct line_ret
{
    size_t index;
    struct line *line;
};

struct line_ret line_insert(struct text *, struct line *, size_t index, char c);
struct line_ret line_delete(struct text *, struct line *, size_t index);
struct line_ret line_backspace(struct text *, struct line *, size_t index);


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

void text_to_str(const struct text *, char *dst, size_t len);
void text_from_str(struct text *, const char *src, size_t len);
