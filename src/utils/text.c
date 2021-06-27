/* text.c
   Rémi Attab (remi.attab@gmail.com), 25 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "text.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// line
// -----------------------------------------------------------------------------

bool line_empty(struct line *line)
{
    return line->c[0] == 0;
}

size_t line_len(struct line *line)
{
    return strnlen(line->c, text_line_cap);
}

struct line_ret line_insert(struct text *text, struct line *line, size_t index, char c)
{
    assert(index <= text_line_cap);

    if (c == '\n') {
        struct line *new = text_insert(text, line);
        memcpy(new->c, line->c + index, text_line_cap - index);
        memset(line->c + index, 0, text_line_cap - index);
        return (struct line_ret) { .line = new, .index = 0 };
    }

    if (line->c[text_line_cap-1]) return (struct line_ret) {0};
    if (line->c[index])
        memmove(line->c + index + 1, line->c + index, text_line_cap - index - 1);
    line->c[index] = c;
    return (struct line_ret) { .line = line, .index = index + 1 };
}

struct line_ret line_delete(struct text *text, struct line *line, size_t index)
{
    assert(index <= text_line_cap);

    if (line->c[index]) {
        memmove(line->c + index, line->c + index + 1, text_line_cap - index);
        line->c[text_line_cap-1] = 0;
        return (struct line_ret) { .line = line, .index = index };
    }

    if (!line->next) return (struct line_ret) {0};

    size_t len = line_len(line->next);
    size_t copy = u64_min(len, text_line_cap - index);
    memcpy(line->c + index, line->next->c, copy);

    if (copy == len) text_erase(text, line->next);
    else memmove(line->next->c, line->next->c + copy, text_line_cap - copy);
    return (struct line_ret) { .line = line, .index = index };
}

struct line_ret line_backspace(struct text *text, struct line *line, size_t index)
{
    assert(index <= text_line_cap);

    if (index) {
        memmove(line->c + index - 1, line->c + index, text_line_cap - index);
        line->c[text_line_cap-1] = 0;
        return (struct line_ret) { .line = line, .index = index - 1 };
    }

    if (!line->prev) return (struct line_ret) {0};

    size_t curr_len = line_len(line);
    size_t prev_len = line_len(line->prev);
    size_t copy = u64_min(curr_len, text_line_cap - prev_len);
    memcpy(line->prev->c + prev_len, line->c, copy);

    if (copy == curr_len) text_erase(text, line);
    else memmove(line->c, line->c + copy, text_line_cap - copy);
    return (struct line_ret) { .line = line->prev, .index = prev_len };
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
    for (size_t i = 0; i != line; ++i, ptr = ptr->next);
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
        memset(at->c, 0, text_line_cap);
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
    const size_t bytes = text_line_cap + 1;
    assert(len >= text->len * bytes);

    for (const struct line *line = text->first; line; line = line->next) {
        memcpy(dst, line->c, bytes);
        dst += bytes;
    }
}

void text_unpack(struct text *text, const char *src, size_t len)
{
    const size_t bytes = text_line_cap + 1;
    assert(len % bytes == 0);
    if (!len) return;

    text_clear(text);
    text_init(text);

    const char *end = src + len;
    struct line *line = text->first;
    while (src < end) {
        memcpy(line->c, src, bytes);
        src += bytes;
        if (src < end) line = text_insert(text, line);
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
        if (j > text_line_cap) continue;
        line->c[j] = src[i];
    }
}
