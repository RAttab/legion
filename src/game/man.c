/* man.c
   Rémi Attab (remi.attab@gmail.com), 10 Feb 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/man.h"
#include "db/res.h"
#include "utils/vec.h"
#include "utils/hash.h"

#include <unistd.h>


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct link link_home(void)
{
    static const char home[] = "/root";
    return man_link(home, sizeof(home) - 1); // gotta ignore the zero char
}

static hash_val link_hash(const char *path, size_t len)
{
    return hash_str(path, legion_min(len, (size_t) man_path_max));
}

static bool link_normalize_path(char *path, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        switch (path[i]) {
        case '/':
        case '-':
        case '0'...'1':
        case 'a'...'z': { break; }
        case 'A'...'Z': { path[i] = tolower(path[i]); break; }
        case ' ': { path[i] = '-'; break; }
        default: { return false; }
        }
    }
    return true;
}


// -----------------------------------------------------------------------------
// man_path
// -----------------------------------------------------------------------------

struct man_path { uint8_t len; char str[man_path_max]; char zero; };

static struct man_path make_man_path(const char *str)
{
    struct man_path path = {0};
    path.len = strnlen(str, man_path_max);
    memcpy(path.str, str, path.len);
    return path;
}

static bool man_path_append(struct man_path *path, const char *str, size_t len)
{
    if (path->len + 1 + len > sizeof(path->str)) return false;

    path->str[path->len] = '/';
    memcpy(path->str + path->len + 1, str, len);
    path->len += 1 + len;

    return link_normalize_path(path->str, path->len);
}


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct man
{
    struct vec32 *lines; // line -> text.list[size_t];
    struct vec16 *sections; // section -> line
    struct { size_t len, cap; char *data; } text;
    struct { size_t len, cap; struct markup *list; } markup;
};

constexpr size_t man_markup_inc = 64;

static const char *man_text_current(struct man *);
static const char *man_text_put(struct man *, char);

static struct man *man_new(size_t text_cap)
{
    text_cap = legion_max(text_cap, page_len);

    struct man *man = calloc(1, sizeof(*man));
    *man = (struct man) {
        .lines = vec32_append(NULL, 0),
        .sections = vec16_append(NULL, 0),

        .text = {
            .len = 0,
            .cap = text_cap,
            .data = calloc(text_cap, sizeof(*man->text.data)),
        },

        .markup = {
            .len = 0,
            .cap = man_markup_inc,
            .list = calloc(man_markup_inc, sizeof(*man->markup.list)),
        },
    };

    man_text_put(man, '\n');

    man->markup.len = 1;
    man->markup.list[0] = (struct markup) {
        .type = markup_nil,
        .text = man_text_current(man),
        .len = 0,
    };

    return man;
}

void man_free(struct man *man)
{
    vec32_free(man->lines);
    vec16_free(man->sections);

    free(man->text.data);
    free(man->markup.list);

    free(man);
}

man_line man_lines(struct man *man)
{
    return man->lines->len;
}

man_line man_section_line(struct man *man, man_section section)
{
    assert(section < man->sections->len);
    return man->sections->vals[section];
}

const struct markup *man_line_markup(struct man *man, man_line line)
{
    line = legion_min(line, man->lines->len - 1);

    size_t index = man->lines->vals[line];
    assert(index < man->markup.len);

    return man->markup.list + index;
}

const struct markup *man_next_markup(struct man *man, const struct markup *it)
{
    const struct markup *start = man->markup.list;
    const struct markup *end = start + man->markup.len;
    assert(it >= start && it < end);
    return it + 1 == end ? NULL : it + 1;
}


static const char *man_text_current(struct man *man)
{
    return man->text.data + man->text.len;
}

static char man_text_previous(struct man *man)
{
    return man->text.data[man->text.len - 1];
}

static const char *man_text_put(struct man *man, char c)
{
    assert(man->text.len + 1 < man->text.cap);
    char *start = man->text.data + man->text.len;

    *start = c;
    man->text.len++;

    return start;
}

static const char *man_text_repeat(struct man *man, char c, size_t len)
{
    assert(man->text.len + len < man->text.cap);
    char *start = man->text.data + man->text.len;

    memset(start, c, len);
    man->text.len += len;

    return start;
}

static const char *man_text_str(struct man *man, const char *text, size_t len)
{
    assert(man->text.len + len < man->text.cap);
    char *start = man->text.data + man->text.len;

    memcpy(start, text, len);
    man->text.len += len;

    return start;
}


static struct markup *man_current(struct man *man)
{
    return man->markup.list + (man->markup.len - 1);
}

static struct markup *man_previous(struct man *man)
{
    assert(man->markup.len > 1);
    return man->markup.list + (man->markup.len - 2);
}

static struct markup *man_markup(struct man *man, enum markup_type type)
{
    struct markup *markup = man_current(man);
    if (markup->type == type) return markup;
    if (!markup->type) {
        markup->type = type;
        return markup;
    }

    if (unlikely(man->markup.len == man->markup.cap)) {
        size_t old = man->markup.cap;
        man->markup.cap += man_markup_inc;
        man->markup.list = realloc_zero(
                man->markup.list, old, man->markup.cap, sizeof(*man->markup.list));
    }

    man->markup.len++;
    markup = man_current(man);
    *markup = (struct markup) {
        .type = type,
        .text = man_text_current(man),
    };
    return markup;
}

static void man_mark_section(struct man *man)
{
    man->sections = vec16_append(man->sections, man->lines->len - 1);
}

static struct markup *man_newline(struct man *man)
{
    // We need to force create an empty markup because this function is used to
    // create paragraphs which requires two consecutives eol which would
    // otherwise be dedupped by man_markup.
    struct markup *markup = man_markup(man, markup_nil);
    *markup = (struct markup) {
        .type = markup_eol,
        .text = man_text_put(man, '\n'),
        .len = 1,
    };

    man->lines = vec32_append(man->lines, man->markup.len);
    return markup;
}

struct link man_click(struct man *man, man_line line, uint8_t col)
{
    for (const struct markup *it = man_line_markup(man, line);
         it; it = man_next_markup(man, it))
    {
        if (it->type == markup_eol) return link_nil();
        if (it->type == markup_link && col < it->len) return it->link;
        col -= it->len;
    }

    return link_nil();
}

void man_dbg(struct man *man)
{
    dbgf("sections(%u):", man->sections->len);
    for (man_section section = 0; section < man->sections->len; ++section)
        dbgf("  %u -> %u", section, man->sections->vals[section]);

    dbgf("lines(%u):", man->lines->len);
    for (man_line line = 0; line < man->lines->len; ++line)
        dbgf("  %u -> %u", line, man->lines->vals[line]);

    dbgf("markup(%zu):", man->markup.len);
    for (size_t i = 0; i < man->markup.len; ++i) {
        const struct markup *it = man->markup.list + i;

        char type = 0;
        switch (it->type)
        {
        case markup_nil: { type = '0'; break; }
        case markup_eol: { type = 'n'; break; }
        case markup_text: { type = 't'; break; }
        case markup_underline: { type = 'u'; break; }
        case markup_bold: { type = 'b'; break; }
        case markup_code_begin: { type = '{'; break; }
        case markup_code: { type = 'c'; break; }
        case markup_code_end: { type = '}'; break; }
        case markup_link: { type = 'l'; break; }
        default: { type = '?'; break; }
        }

        dbgf("  %03zu: %p, type=%c, lk={%02u,%02u}, len=%02u, text=%.*s",
                i, it, type, it->link.page, it->link.section,
                it->len, (unsigned) it->len, it->text);
    }
}


// -----------------------------------------------------------------------------
// toc
// -----------------------------------------------------------------------------

static struct toc *toc_path_it(
        struct toc *toc, const struct man_path *path, size_t start)
{
    if (start == path->len) return toc;

    assert(start < path->len);
    assert(path->str[start] == '/');

    size_t end = ++start;
    for (; end < path->len && path->str[end] != '/'; ++end);

    size_t len = end - start;
    const char *name = path->str + start;
    if (len >= sizeof(toc->name)) {
        dbgf("man: toc path '%.*s' too long", (unsigned) len, name);
        abort();
    }


    for (size_t i = 0; i < toc->len; ++i) {
        struct toc *node = toc->nodes + i;
        if (!strncmp(node->name, name, len))
            return toc_path_it(node, path, end);
    }

    if (unlikely(toc->len == toc->cap)) {
        size_t cap = toc->cap ? toc->cap * 2 : 4;
        toc->nodes = realloc_zero(toc->nodes, toc->cap, cap, sizeof(toc->nodes[0]));
        toc->cap = cap;
    }

    struct toc *node = toc->nodes + toc->len;
    memcpy(node->name, name, len);
    toc->len++;
    return toc_path_it(node, path, end);
}

static struct toc *toc_path(struct toc *toc, struct man_path path)
{
    return toc_path_it(toc, &path, 0);
}

static void toc_sort(struct toc *toc)
{
    int cmp(const void *lhs_, const void *rhs_)
    {
        const struct toc *lhs = lhs_;
        const struct toc *rhs = rhs_;
        return strncmp(lhs->name, rhs->name, sizeof(lhs->name));
    }
    qsort(toc->nodes, toc->len, sizeof(toc->nodes[0]), cmp);
}


// -----------------------------------------------------------------------------
// man_page
// -----------------------------------------------------------------------------

struct man_page
{
    man_page page;
    enum item item;
    const char *path;

    size_t data_len;
    const char *data;
};

static struct man *man_page_render(const struct man_page *, uint8_t cols, struct lisp *);
static bool man_page_index(struct man_page *, struct toc *, struct atoms *);


// -----------------------------------------------------------------------------
// mans
// -----------------------------------------------------------------------------

static struct
{
    struct toc toc;
    struct htable index; // hash(path) -> link_to_u64(struct link)
    struct { size_t len, cap; struct man_page *list; } pages;
} mans;

struct link man_path(const char *path, size_t len)
{
    hash_val key = link_hash(path, len);
    struct htable_ret ret = htable_get(&mans.index, key);
    return ret.ok ? link_from_u64(ret.value) : link_nil();
}

struct man *man_open(man_page page, uint8_t cols, struct lisp *lisp)
{
    if (!page) return NULL;
    assert(cols);
    assert(lisp);

    size_t index = page - 1;
    assert(index < mans.pages.len);

    struct man_page *man = mans.pages.list + index;
    return man_page_render(man, cols, lisp);
}

const struct toc *man_toc(void)
{
    return &mans.toc;
}

struct link man_link(const char *path, size_t len)
{
    hash_val key = link_hash(path, len);
    struct htable_ret ret = htable_get(&mans.index, key);
    return ret.ok ? link_from_u64(ret.value) : link_nil();
}

struct link man_sys_locked(void)
{
    static const char path[] = "/sys/locked";
    return man_link(path, sizeof(path) - 1);
}

enum item man_item(man_page page)
{
    size_t index = page - 1;
    assert(index < mans.pages.len);

    struct man_page *man = mans.pages.list + index;
    return man->item;
}

static void man_index_path(const char *path, size_t len, struct link link)
{
    hash_val key = link_hash(path, len);
    struct htable_ret ret = htable_put(&mans.index, key, link_to_u64(link));
    assert(ret.ok);
}

static bool man_populate_db(struct atoms *atoms)
{
    bool ok = true;
    struct db_man_it it = {0};

    while (db_man_next(&it)) {
        if (mans.pages.len == mans.pages.cap) {
            size_t cap = mans.pages.cap ? mans.pages.cap * 2 : 8;
            mans.pages.list = realloc_zero(
                    mans.pages.list,
                    mans.pages.cap, cap,
                    sizeof(*mans.pages.list));
            mans.pages.cap = cap;
        }

        size_t index = mans.pages.len;
        mans.pages.len++;

        struct man_page *page = mans.pages.list + index;
        page->page = index + 1;
        page->path = it.path;
        page->data = it.data;
        page->data_len = it.data_len;

        ok = man_page_index(page, &mans.toc, atoms) && ok;
    }

    return ok;
}

void man_populate(struct atoms *atoms)
{
    // Pre-create key nodes in the toc to enforce logical ordering
    struct toc *root = &mans.toc;
    (void) toc_path(root, make_man_path("/root"));
    (void) toc_path(root, make_man_path("/guides"));
    (void) toc_path(root, make_man_path("/guides/factory"));
    (void) toc_path(root, make_man_path("/guides/mods"));
    (void) toc_path(root, make_man_path("/guides/debug"));
    (void) toc_path(root, make_man_path("/guides/flow"));
    (void) toc_path(root, make_man_path("/guides/variables"));
    (void) toc_path(root, make_man_path("/guides/introspection"));
    (void) toc_path(root, make_man_path("/guides/io-return"));
    (void) toc_path(root, make_man_path("/concepts"));
    (void) toc_path(root, make_man_path("/concepts/stars"));
    (void) toc_path(root, make_man_path("/concepts/factory"));
    (void) toc_path(root, make_man_path("/concepts/cpu"));
    (void) toc_path(root, make_man_path("/concepts/lisp"));
    (void) toc_path(root, make_man_path("/items"));
    (void) toc_path(root, make_man_path("/lisp"));
    (void) toc_path(root, make_man_path("/asm"));

    if (!man_populate_db(atoms))
        fail("unable to index man pages");

    // Sort the toc nodes to enfore sane ordering on the reference pages.
    toc_sort(toc_path(root, make_man_path("/items")));
    toc_sort(toc_path(root, make_man_path("/lisp")));
    toc_sort(toc_path(root, make_man_path("/asm")));
}


// -----------------------------------------------------------------------------
// implementations
// -----------------------------------------------------------------------------

#include "man_parser.c"
#include "man_index.c"
#include "man_render.c"
