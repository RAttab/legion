/* man.h
   Rémi Attab (remi.attab@gmail.com), 10 Feb 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/symbol.h"

struct lisp;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

enum { man_path_max = 64, man_toc_max = 16 };

typedef uint16_t line_t;
typedef uint16_t page_t;
typedef uint16_t section_t;


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct link { page_t page; section_t section; };

inline bool link_is_nil(struct link link) { return link.page == 0; }

inline struct link make_link(page_t page, section_t section)
{
    return (struct link) { .page = page, .section = section };
}

inline struct link link_nil(void) { return make_link(0, 0); }

struct link link_home(void);

inline uint64_t link_to_u64(struct link link)
{
    return ((uint64_t) link.page << 32) | link.section;
}

inline struct link link_from_u64(uint64_t value)
{
    return (struct link) { .page = value >> 32, .section = value };
}


// -----------------------------------------------------------------------------
// markup
// -----------------------------------------------------------------------------

enum legion_packed markup_type
{
    markup_nil = 0,
    markup_eol,
    markup_text,
    markup_underline,
    markup_bold,
    markup_code,
    markup_link,
};

struct markup
{
    enum markup_type type;
    struct link link;
    uint16_t len, cap;
    char *text;
};


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct man;

struct man *man_page(page_t page, uint8_t cols, struct lisp *lisp);
void man_free(struct man *);

line_t man_lines(struct man *);
line_t man_section(struct man *, section_t);

const struct markup *man_line(struct man *, line_t);
const struct markup *man_next(struct man *, const struct markup *);

struct link man_link(const char *path, size_t len);
struct link man_click(struct man *, line_t, uint8_t col);

struct toc
{
    struct link link;
    char name[man_toc_max];

    uint8_t len, cap;
    struct toc *nodes;
};
const struct toc *man_toc(void); // The Mind Taker! BWWWWWEEEEOOOO!

void man_populate(void);
