/* man.c
   Rémi Attab (remi.attab@gmail.com), 10 Feb 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/man.h"
#include "utils/vec.h"
#include "utils/hash.h"


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct link link_home(void)
{
    static const char home[] = "/root";
    return man_link(home, sizeof(home) - 1); // gotta ignore the zero char
}

static hash_t link_hash(const char *path, size_t len)
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
// markup
// -----------------------------------------------------------------------------

static void markup_free(struct markup *markup)
{
    free(markup->text);
}

static void markup_grow(struct markup *markup, size_t len)
{
    if (likely(markup->len + len <= markup->cap)) return;

    if (!markup->cap) markup->cap = 8;
    while (markup->cap < markup->len + len) markup->cap *= 2;

    markup->text = realloc(markup->text, markup->cap);
}

static void markup_repeat(struct markup *markup, char c, size_t len)
{
    markup_grow(markup, len);
    memset(markup->text + markup->len, c, len);
    markup->len += len;
}

static void markup_str(struct markup *markup, const char *str, size_t len)
{
    markup_grow(markup, len);
    memcpy(markup->text + markup->len, str, len);
    markup->len += len;
}

static void markup_continue(struct markup *dst, const struct markup *src)
{
    *dst = (struct markup) {
        .type = src->type,
        .link = src->link,
    };
}


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct man
{
    struct vec64 *lines; // line_t -> text.list[size_t];
    struct vec64 *sections; // section_t -> line_t
    struct { size_t len, cap; struct markup *list; } text;
};

static struct man *man_new(void)
{
    struct man *man = calloc(1, sizeof(*man));

    man->text.cap = 8;
    man->text.list = calloc(man->text.cap, sizeof(*man->text.list));

    man->text.len = 1;
    assert(man->text.list[0].type == markup_nil);

    man->lines = vec64_append(man->lines, 0);
    man->sections = vec64_append(man->sections, 0);

    return man;
}

void man_free(struct man *man)
{
    vec64_free(man->lines);
    vec64_free(man->sections);

    for (size_t i = 0; i < man->text.len; ++i)
        markup_free(man->text.list + i);
    free(man->text.list);

    free(man);
}

line_t man_lines(struct man *man)
{
    return man->lines->len;
}

line_t man_section(struct man *man, section_t section)
{
    assert(section < man->sections->len);
    return man->sections->vals[section];
}

const struct markup *man_line(struct man *man, line_t line)
{
    line = legion_min(line, man->lines->len - 1);
    return man->text.list + man->lines->vals[line];
}

const struct markup *man_next(struct man *man, const struct markup *it)
{
    const struct markup *start = man->text.list;
    const struct markup *end = start + man->text.len;
    assert(it >= start && it < end);
    return it + 1 == end ? NULL : it + 1;
}

static struct markup *man_current(struct man *man)
{
    assert(man->text.len);
    return man->text.list + (man->text.len - 1);
}

static struct markup *man_previous(struct man *man)
{
    assert(man->text.len > 1);
    return man->text.list + (man->text.len - 2);
}

static struct markup *man_markup(struct man *man, enum markup_type type)
{
    struct markup *markup = man_current(man);
    if (markup->type == type) return markup;
    if (!markup->type) {
        markup->type = type;
        return markup;
    }

    if (unlikely(man->text.len == man->text.cap)) {
        size_t cap = man->text.cap ? man->text.cap * 2 : 8;
        man->text.list = realloc_zero(
                man->text.list, man->text.cap, cap, sizeof(man->text.list[0]));
        man->text.cap = cap;
    }
    man->text.len++;

    markup = man_current(man);
    markup->type = type;
    assert(!markup->cap && !markup->len && !markup->text);
    return markup;
}

static void man_mark_section(struct man *man)
{
    man->sections = vec64_append(man->sections, man->lines->len - 1);
}

static struct markup *man_newline(struct man *man)
{
    // Required in the case where we have two consecutives eol to create a
    // paragraph.
    struct markup *markup = man_markup(man, markup_nil);
    markup->type = markup_eol;

    man->lines = vec64_append(man->lines, man->text.len);
    return markup;
}

struct link man_click(struct man *man, line_t line, uint8_t col)
{
    for (const struct markup *it = man_line(man, line);
         it; it = man_next(man, it))
    {
        if (it->type == markup_eol) return link_nil();
        if (it->type == markup_link && col < it->len) return it->link;
        col -= it->len;
    }

    return link_nil();
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
    assert(len < sizeof(toc->name));
    const char *name = path->str + start;

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
    page_t page;
    struct mfile file;
    char path[PATH_MAX+1];
};

static struct man *man_page_render(const struct man_page *, uint8_t cols, struct lisp *);
static bool man_page_index(const struct man_page *, struct toc *);


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
    hash_t key = link_hash(path, len);
    struct htable_ret ret = htable_get(&mans.index, key);
    return ret.ok ? link_from_u64(ret.value) : link_nil();
}

struct man *man_page(page_t page, uint8_t cols, struct lisp *lisp)
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
    hash_t key = link_hash(path, len);
    struct htable_ret ret = htable_get(&mans.index, key);
    return ret.ok ? link_from_u64(ret.value) : link_nil();
}

static void man_index_path(const char *path, size_t len, struct link link)
{
    hash_t key = link_hash(path, len);
    struct htable_ret ret = htable_put(&mans.index, key, link_to_u64(link));
    assert(ret.ok);
}

static bool man_populate_it(const char *base)
{
    bool ok = true;
    struct dir_it *it = dir_it(base);

    while (dir_it_next(it)) {
        const char *path = dir_it_path(it);

        if (path_is_dir(path))
            ok = man_populate_it(path) && ok;

        else if (path_is_file(path)) {
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
            page->file = mfile_open(path);
            strncpy(page->path, path, sizeof(page->path) - 1);

            ok = man_page_index(page, &mans.toc) && ok;
        }
    }

    dir_it_free(it);
    return ok;
}

void man_populate(void)
{
    // Pre-create key nodes in the toc to enforce logical ordering
    struct toc *root = &mans.toc;
    (void) toc_path(root, make_man_path("/root"));
    (void) toc_path(root, make_man_path("/concepts"));
    (void) toc_path(root, make_man_path("/concepts/cpu"));
    (void) toc_path(root, make_man_path("/concepts/io"));
    (void) toc_path(root, make_man_path("/asm"));
    (void) toc_path(root, make_man_path("/lisp"));
    (void) toc_path(root, make_man_path("/items"));

    char path[PATH_MAX] = {0};
    sys_path_res("man", path, sizeof(path));
    if (!man_populate_it(path))
        fail("unable to index man pages");

    // Sort the toc nodes to enfore sane ordering on the reference pages.
    toc_sort(toc_path(root, make_man_path("/asm")));
    toc_sort(toc_path(root, make_man_path("/lisp")));
    toc_sort(toc_path(root, make_man_path("/items")));
}


// -----------------------------------------------------------------------------
// implementations
// -----------------------------------------------------------------------------

#include "man_parser.c"
#include "man_index.c"
#include "man_render.c"
