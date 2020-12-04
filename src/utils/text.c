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
    return strnlen(line->c, line_cap);
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
    if (line > text->len) return NULL;

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

    if (at == text->last) text->last = new;
    text->len++;
    return new;
}

struct line *text_erase(struct text *text, struct line *at)
{
    if (at == text->first && at == text->last) {
        memset(at->c, 0, line_cap);
        return at;
    }

    struct line *ret = at->next;
    if (at->prev) at->prev->next = at->next;
    if (at->next) at->next->prev = at->prev;
    if (at == text->first) text->first = at->next;
    if (at == text->last) text->last = at->prev;

    free(at);
    text->len--;
    return ret;
}

void text_pack(const struct text *text, char *dst, size_t len)
{
    const size_t bytes = line_cap + 1;
    assert(len >= text->len * bytes);

    for (const struct line *line = text->first; line; line = line->next) {
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

void text_to_str(const struct text *text, char *dst, size_t len)
{
    size_t i = 0;
    struct line *line = text->first;
    for (size_t j = 0; line && i < len; ++i, ++j) {
        if (line->c[j]) { dst[i] = '\n'; j = 0; line = line->next; }
        else dst[i] = line->c[j];
    }
    dst[i] = 0;
}

void text_from_str(struct text *text, const char *src, size_t len)
{
    if (!len) return;
    text_clear(text);
    text_init(text);

    struct line *line = text->first;
    for (size_t i = 0, j = 0; i < len; ++i, ++j) {
        if (src[i] == '\n') { line = text_insert(text, line); j = -1; continue; }
        if (j > line_cap) continue;
        line->c[j] = src[i];
    }
}
