/* text.c
   Rémi Attab (remi.attab@gmail.com), 25 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "text.h"

// -----------------------------------------------------------------------------
// line
// -----------------------------------------------------------------------------

bool line_empty(struct line *line)
{
    return line->c[0] == 0;
}

size_t line_len(struct line *line)
{
    return strnlen(line, line_cap);
}

void line_put(struct line *line, size_t index, char c)
{
    assert(index < line_cap);

    for (size_t i = 0; i < index; ++i)
        if (!line->c[index]) line->c[index] = ' ';
    line->c[index] = c;
}

void line_del(struct line *line, size_t index)
{
    assert(index < line_cap);
    memmove(&line->c[index], &line->c[index+1], line_cap - index - 1);
    line->c[line_cap-1] = 0;
}


// -----------------------------------------------------------------------------
// text
// -----------------------------------------------------------------------------

void text_init(struct text *text)
{
    text->first = text->last = alloc_cache(sizeof(*text->first));
    text->len = 1;
}

void text_clear(struct text *text)
{
    struct line *line = text->first;
    while (line) {
        struct line *next = line->next;
        free(line);
        line = next;
    }

    text->first = text->last = NULL;
}

struct line *text_goto(struct text *text, size_t line)
{
    if (line > text->line) return NULL;

    struct line *ptr = text->first;
    for (size_t i = 1; i != line; ++i, ptr = ptr->next);
    return ptr;
}

struct line *text_insert(struct text *text, struct line *at)
{
    struct line *new = alloc_cache(sizeof(*at));

    if (at->next) at->next->prev = new;
    new->next = at->next;
    new->prev = at;
    at->next = new;

    if (at == last) last = new;
    text->line++;
    return new;
}

struct line *text_erase(struct text *text, struct line *at)
{
    if (at == first && at == last) {
        memset(at->c, 0, line_cap);
        return at;
    }

    if (at->prev) at->prev->next = at->next;
    if (at->next) at->next->prev = at->prev;
    if (at == text->first) text->first = at->next;
    if (at == text->last) text->last = at->prev;
    if (at == text->curr) text->curr = text->first;

    free(at);
    text->line--;
}

void text_pack(struct text *text, char *dst, size_t len)
{
    const size_t bytes = line_cap + 1;
    assert(len >= text->len * bytes);

    for (struct line *line = text->first; line; line = line->next) {
        memcpy(dst, line->c, bytes);
        dst += bytes;
    }
}

void text_unpack(struct text *text, const char *src, size_t len)
{
    const size_t bytes = line_cap + 1;
    assert(len % bytes == 0);
    if (!len) return;

    text_clear(text);
    text_init(text);

    const char *end = src + len;
    struct line *line = text->first;
    while (true) {
        memcpy(line->c, src, bytes);
        if ((src += bytes) == end) break;
        line = text_insert(text, line);
    }
}
