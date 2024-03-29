/* man.h
   Rémi Attab (remi.attab@gmail.com), 10 Feb 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct lisp;
struct atoms;


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

enum : man_section { link_ui_tape = 0xFFFF };

inline bool link_is_nil(struct link link) { return link.page == 0; }

inline struct link make_link(man_page page, man_section section)
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

enum markup_type : uint8_t
{
    markup_nil = 0,
    markup_eol,
    markup_text,
    markup_underline,
    markup_bold,
    markup_code_begin,
    markup_code,
    markup_code_end,
    markup_link,
};

struct legion_packed markup
{
    struct link link;
    enum markup_type type;
    uint8_t len;
    legion_pad(2);
    const char *text;
};


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct man;

struct man *man_open(man_page page, uint8_t cols, struct lisp *lisp);
void man_free(struct man *);

man_line man_lines(struct man *);
man_line man_section_line(struct man *, man_section);
const struct markup *man_line_markup(struct man *, man_line);
const struct markup *man_next_markup(struct man *, const struct markup *);

struct link man_link(const char *path, size_t len);
struct link man_link_ui(const char *path, size_t len);
struct link man_click(struct man *, man_line, uint8_t col);

enum item man_item(man_page page);
struct link man_sys_locked(void);

void man_dbg(struct man *);

struct toc
{
    struct link link;
    char name[man_toc_max];

    enum item item;

    uint8_t len, cap;
    struct toc *nodes;
};
const struct toc *man_toc(void); // The Mind Taker! BWWWWWEEEEOOOO!

void man_populate(struct atoms *);
