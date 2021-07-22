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

static struct line *line_realloc(
        struct text *text, struct line *old, size_t len)
{
    assert(old);
    if (likely(old->cap >= len)) return old;

    size_t cap = old->cap;
    while (cap < len) cap += s_cache_line;
    struct line *new = realloc(old, sizeof(*new) + cap);
    new->cap = cap;

    if (new->next) new->next->prev = new;
    if (new->prev) new->prev->next = new;
    if (text->first == old) text->first = new;
    if (text->last == old) text->last = new;
    return new;
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

static size_t line_indent_spaces(size_t indent) { return indent * 2; }

static struct line *line_indent(
        struct text *text, struct line *line, size_t indent, bool new)
{
    indent = line_indent_spaces(indent);

    size_t first = 0;
    while (first < line->len && line->c[first] <= 0x20) first++;

    if (!new && first == line->len) {
        line->len = 0;
        return line;
    }

    ssize_t delta = ((ssize_t) indent) - ((ssize_t) first);
    if (!delta) return line;

    line = line_realloc(text, line, line->len + delta);
    memmove(line->c + indent, line->c + first, line->len - first);
    memset(line->c, ' ', indent);

    line->len += delta;
    text->bytes += delta;
    return line;
}

struct line_ret line_insert(
        struct text *text, struct line *line, size_t index, char c)
{
    assert(index <= line->len);
    text->bytes++;

    if (likely(c != '\n')) {
        assert(line->len + 1 < UINT16_MAX);

        line = line_realloc(text, line, line->len + 1);
        memmove(line->c + index + 1, line->c + index, line->len - index);

        line->c[index] = c;
        line->len++;
        return (struct line_ret) { .line = line, .new = NULL, .index = index + 1 };
    }

    size_t to_copy = line->len - index;
    struct line *new = text_insert(text, line);
    new = line_realloc(text, new, to_copy);

    memcpy(new->c, line->c + index, to_copy);
    new->len = to_copy;
    line->len = index;

    size_t indent = text_indent_at(text, new);
    new = line_indent(text, new, indent, true);
    size_t spaces = line_indent_spaces(indent);

    return (struct line_ret) { .line = line, .new = new, .index = spaces };
}

struct line_ret line_delete(struct text *text, struct line *line, size_t index)
{
    assert(index <= line->len);

    if (likely(index < line->len)) {
        memmove(line->c + index, line->c + index + 1, line->len - index - 1);
        line->len--;
        text->bytes--;
        return (struct line_ret) { .line = line, .new = NULL, .index = index };
    }

    struct line *next = line->next;
    if (!next) return (struct line_ret) {0};

    size_t to_copy = next->len;
    assert(line->len + to_copy < UINT16_MAX);

    line = line_realloc(text, line, line->len + next->len);
    memcpy(line->c + line->len, next->c, to_copy);
    line->len += to_copy;

    text_erase(text, line->next);
    text->bytes += to_copy;

    return (struct line_ret) { .line = line, .new = NULL, .index = index };
}

struct line_ret line_backspace(struct text *text, struct line *line, size_t index)
{
    assert(index <= line->len);

    if (index) {
        memmove(line->c + index - 1, line->c + index, line->len - index);
        line->len--;
        text->bytes--;
        return (struct line_ret) { .line = line, .new = NULL, .index = index - 1 };
    }

    struct line *prev = line->prev;
    if (!prev) return (struct line_ret) {0};

    size_t prev_len = prev->len;
    size_t to_copy = line->len;
    assert(prev->len + to_copy < UINT16_MAX);

    prev = line_realloc(text, prev, prev->len + to_copy);
    memcpy(prev->c + prev->len, line->c, to_copy);
    prev->len += to_copy;

    text_erase(text, line);
    text->bytes += to_copy;

    return (struct line_ret) { .line = NULL, .new = prev, .index = prev_len };
}

void line_setc(struct text *text, struct line *line, size_t len, const char *str)
{
    ssize_t delta = len - line->len;
    text->bytes += delta;

    line = line_realloc(text, line, len);
    memcpy(line->c, str, len);
    line->len = len;
}

void line_setf(struct text *text, struct line *line, size_t cap, const char *fmt, ...)
{
    line = line_realloc(text, line, cap);

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
        line = line_realloc(text, line, line->len);
        memcpy(line->c, start, line->len);
        text->bytes += line->len;

        if (*src == '\n') {
            line = text_insert(text, line);
            src++;
        }
    }
}
