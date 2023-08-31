/* symbol.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/hash.h"

#include <ctype.h>

struct save;

// -----------------------------------------------------------------------------
// symbol
// -----------------------------------------------------------------------------

constexpr size_t symbol_cap = 30;

struct legion_packed symbol
{
    uint8_t len;
    char c[symbol_cap];
    char zero;
};
static_assert(sizeof(struct symbol) == 32);

struct symbol make_symbol(const char *str);
struct symbol make_symbol_len(const char *str, size_t len);

inline hash_val symbol_hash(const struct symbol *symbol)
{
    return hash_bytes(hash_init(), symbol->c, symbol->len);
}

#define symbol_hash_c(str)                                              \
    ({                                                                  \
        static_assert(__builtin_constant_p(str));                       \
        struct symbol symbol = make_symbol_len((str), sizeof(str));     \
        uint64_t hash = symbol_hash(&symbol);                           \
        hash;                                                           \
    })

inline int symbol_cmp(const struct symbol *lhs, const struct symbol *rhs)
{
    int ret = memcmp(lhs->c, rhs->c, legion_min(lhs->len, rhs->len));
    return likely(ret) ? ret : lhs->len < rhs->len;
}

inline bool symbol_eq(const struct symbol *lhs, const struct symbol *rhs)
{
    if (lhs->len != rhs->len) return false;
    return symbol_cmp(lhs, rhs) == 0;
}

struct symbol symbol_concat(const char *lhs, const char *rhs);

inline bool symbol_char(char c)
{
    switch (c) {
    case '@': case '_': case '*':
    case '-': case '+': case '/':
    case '=': case '<': case '>':
    case 'a'...'z': case 'A'...'Z':
    case '0'...'9':
        return true;

    default: return false;
    }
}

ssize_t symbol_parse(const char *it, size_t len, struct symbol *value);

void symbol_save(const struct symbol *sym, struct save *);
bool symbol_load(struct symbol *sym, struct save *);
