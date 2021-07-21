/* text.c
   Rémi Attab (remi.attab@gmail.com), 25 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "text.h"
#include "utils/bits.h"
#include <stdarg.h>


// -----------------------------------------------------------------------------
// line
// -----------------------------------------------------------------------------

enum { line_default_cap = s_cache_line - sizeof(struct line) };

static struct line *line_alloc(void)
{
    struct line *line = calloc(1, sizeof(*line) + line_default_cap);
    line->cap = line_default_cap;
    return line;
}

static struct line *line_realloc(struct line *line, size_t len)
{
    assert(line);
    if (likely(line->cap >= len)) return line;

    size_t cap = line->cap;
    while (cap < len) cap += s_cache_line;
    line = realloc(line, sizeof(*line) + cap);
    line->cap = cap;

    if (line->next) line->next->prev = line;
    if (line->prev) line->prev->next = line;
    return line;
}

static int16_t line_indent_delta(struct line *line)
{
    int16_t indent = 0;
    for (size_t i = 0; i < line->len; ++i) {
        switch (line->c[i]) {
        case '(': { indent++; break; }
        case ')': { indent--; break; }
        default: { break; }
        }
    }
    return indent;
}

static void line_indent(
        struct text *text, struct line *line, size_t indent, bool new)
{
    size_t first = 0;
    while (first < line->len && line->c[first] <= 0x20) first++;

    if (!new && first == line->len) {
        line->len = 0;
        return;
    }

    ssize_t delta = ((ssize_t) indent) - ((ssize_t) first);
    if (!delta) return;

    line = line_realloc(line, line->len + delta);
    memmove(line->c + indent, line->c + first, line->len - first);
    memset(line->c, ' ', indent);

    line->len += delta;
    text->bytes += delta;
}

struct line_ret line_insert(
        struct text *text, struct line *line, size_t index, char c)
{
    assert(index <= line->len);
    text->bytes++;

    if (likely(c != '\n')) {
        assert(line->len + 1 < UINT16_MAX);

        line = line_realloc(line, line->len + 1);
        memmove(line->c + index + 1, line->c + index, line->len - index);

        line->c[index] = c;
        line->len++;
        return (struct line_ret) { .line = line, .index = index + 1 };
    }

    struct line *new = text_insert(text, line);
    memcpy(new->c, line->c + index, line->len - index);
    new->len = line->len - index;
    line->len = index;

    size_t indent = text_indent_at(text, new);
    line_indent(text, line, indent, true);

    return (struct line_ret) { .line = new, .index = indent };
}

struct line_ret line_delete(struct text *text, struct line *line, size_t index)
{
    assert(index <= line->len);

    if (likely(index < line->len)) {
        memmove(line->c + index, line->c + index + 1, line->len - index - 1);
        line->len--;
        text->bytes--;
        return (struct line_ret) { .line = line, .index = index };
    }

    struct line *next = line->next;
    if (!next) return (struct line_ret) {0};

    size_t to_copy = next->len;
    assert(line->len + to_copy < UINT16_MAX);

    line = line_realloc(line, line->len + next->len);
    memcpy(line->c + line->len, next->c, to_copy);
    line->len += to_copy;

    text_erase(text, line->next);
    text->bytes += to_copy;

    return (struct line_ret) { .line = line, .index = index };
}

struct line_ret line_backspace(struct text *text, struct line *line, size_t index)
{
    assert(index <= line->len);

    if (index) {
        memmove(line->c + index - 1, line->c + index, line->len - index);
        line->len--;
        text->bytes--;
        return (struct line_ret) { .line = line, .index = index - 1 };
    }

    struct line *prev = line->prev;
    if (!prev) return (struct line_ret) {0};

    size_t to_copy = line->len;
    assert(prev->len + to_copy < UINT16_MAX);

    prev = line_realloc(prev, prev->len + to_copy);
    memcpy(prev->c + prev->len, line->c, to_copy);
    prev->len += to_copy;

    text_erase(text, line);
    text->bytes += to_copy;

    return (struct line_ret) { .line = prev, .index = prev->len };
}

void line_setc(struct text *text, struct line *line, size_t len, const char *str)
{
    ssize_t delta = len - line->len;
    text->bytes += delta;

    line = line_realloc(line, len);
    memcpy(line->c, str, len);
    line->len = len;
}

void line_setf(struct text *text, struct line *line, size_t cap, const char *fmt, ...)
{
    line = line_realloc(line, cap);

    va_list args = {0};
    va_start(args, fmt);
    size_t len = vsnprintf(line->c, line->cap, fmt, args);
    va_end(args);

    ssize_t delta = len - line->len;
    line->len = len;
    text->bytes += delta;
}


// -----------------------------------------------------------------------------
// text
// -----------------------------------------------------------------------------

void text_init(struct text *text)
{
    text->first = text->last = line_alloc();
    text->bytes = 1;
    text->lines = 1;
}

void text_clear(struct text *text)
{
    struct line *line = text->first;
    while (line) {
        struct line *next = line->next;
        free(line);
        line = next;
    }

    text->lines = text->bytes = 0;
    text->first = text->last = NULL;
}

struct line *text_goto(struct text *text, size_t line)
{
    if (line > text->lines) return NULL;

    struct line *ptr = text->first;
    for (size_t i = 0; i != line; ++i, ptr = ptr->next);
    return ptr;
}

struct line *text_insert(struct text *text, struct line *at)
{
    struct line *new = line_alloc();

    if (at->next) at->next->prev = new;
    new->next = at->next;
    new->prev = at;
    at->next = new;

    if (at == text->last) text->last = new;
    text->lines++;
    text->bytes++;
    return new;
}

struct line *text_erase(struct text *text, struct line *at)
{
    if (at == text->first && at == text->last) {
        text->bytes -= at->len;
        at->len = 0;
        return at;
    }

    struct line *ret = at->next;
    if (at->prev) at->prev->next = at->next;
    if (at->next) at->next->prev = at->prev;
    if (at == text->first) text->first = at->next;
    if (at == text->last) text->last = at->prev;

    text->bytes -= at->len + 1;
    text->lines--;

    free(at);
    return ret;
}


void text_indent(struct text *text)
{
    int16_t indent = 0;
    for (struct line *it = text->first; it; it = it->next) {
        line_indent(text, it, indent > 0 ? indent * 2 : 0, false);
        indent += line_indent_delta(it);
    }
}

size_t text_indent_at(struct text *text, struct line *line)
{
    int16_t indent = 0;
    for (struct line *it = text->first; it && it != line; it = it->next)
        indent += line_indent_delta(it);
    return indent;
}


size_t text_to_str(const struct text *text, char *dst, size_t len)
{
    assert(len >= text->bytes);

    size_t written = 0;
    char *end = dst + len;

    for (struct line *it = text->first; it; it = it->next) {
        const size_t to_write = it->len + (it->next ? 1 : 0);
        assert(dst + to_write <= end);

        memcpy(dst, it->c, it->len);
        dst += it->len;

        *dst = '\n';
        dst++;

        written += to_write;
    }

    return written;
}

void text_from_str(struct text *text, const char *src, size_t len)
{
    if (!len) return;
    text_clear(text);
    text_init(text);
    text->bytes = 0;

    const char *end = src + len;
    struct line *line = text->first;

    while (src < end) {
        const char *start = src;
        while (src < end && *src != '\n') src++;

        line->len = src - start;
        line = line_realloc(line, line->len);
        memcpy(line->c, start, line->len);
        text->bytes += line->len;

        if (*src == '\n') {
            line = text_insert(text, line);
            src++;
        }
    }
}
