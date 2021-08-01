/* symbol.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include <ctype.h>


// -----------------------------------------------------------------------------
// symbol
// -----------------------------------------------------------------------------

enum { symbol_cap = 30 };

struct legion_packed symbol
{
    uint8_t len;
    char c[symbol_cap];
    char zero;
};
static_assert(sizeof(struct symbol) == 32);

struct symbol make_symbol(const char *str);
struct symbol make_symbol_len(const char *str, size_t len);

// FNV-1a hash implementation: http://isthe.com/chongo/tech/comp/fnv/
inline uint64_t symbol_hash(const struct symbol *symbol)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < symbol->len; ++i)
        hash = (hash ^ symbol->c[i]) * 0x100000001b3;
    return hash;
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

size_t symbol_parse(const char *it, size_t len, struct symbol *value);
