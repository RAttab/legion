/* types.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// declarations
// -----------------------------------------------------------------------------

struct vm;
struct mod;
struct text;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef int64_t word_t;
typedef uint8_t reg_t;

typedef uint32_t ip_t;
typedef uint32_t mod_t;
typedef uint16_t mod_id_t;
typedef uint16_t mod_ver_t;

inline mod_t make_mod(mod_id_t id, mod_ver_t ver) { assert(!(id >> 15)); return id << 16 | ver; }
inline mod_id_t mod_id(mod_t mod) { return mod >> 16; }
inline mod_ver_t mod_ver(mod_t mod) { return ((1 << 16) - 1) & mod; }

inline ip_t mod_ip(mod_t mod) { return 1U << 31 | mod; }
inline bool ip_is_mod(ip_t ip) { return ip >> 31; }
inline mod_t ip_mod(ip_t ip) { return ((1U << 31) - 1) & ip;  }


// -----------------------------------------------------------------------------
// symbol
// -----------------------------------------------------------------------------

enum { symbol_cap = 30 };

struct symbol
{
    uint8_t len;
    char c[symbol_cap];
    char zero;
};
static_assert(sizeof(struct symbol) == 32);

inline void symbol_normalize(struct symbol *symbol)
{
    memset(symbol->c, 0, symbol_cap - symbol->len);
    for (size_t i = 0; i < symbol->len; ++i)
        symbol->c[i] = tolower(symbol->c[i]);
}


inline struct symbol make_symbol(const char *str)
{
    struct symbol symbol = { .len = strnlen(str, symbol_cap), .zero = 0 };
    memcpy(symbol.c, str, symbol.len);
    symbol_normalize(symbol);
    return symbol;
}

inline struct symbol make_symbol_len(size_t len, const char *str)
{
    struct symbol symbol = { .len = len, .zero = 0 };
    memcpy(symbol.c, str, symbol.len);
    symbol_normalize(symbol);
    return symbol;
}

// FNV-1a hash implementation: http://isthe.com/chongo/tech/comp/fnv/
inline uint64_t symbol_hash(const struct symbol *symbol)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < symbol->len; ++i)
        hash = (hash ^ symbol->c[i]) * 0x100000001b3;
    return hash;
}

inline int symbol_cmp(const struct symbol *lhs, const struct symb *rhs)
{
    int ret = memcmp(lhs->c, rhs->c, legion_min(lhs->len, rhs->len));
    return likely(ret) ? ret : lhs->len < rhs->len;
}

inline bool symbol_eq(const struct symbol *lhs, const struct symb *rhs)
{
    if (lhs->len != rhs->len) return false;
    return symbol_cmp(lhs, rhs) == 0;
}


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

struct atoms;

struct atoms *atoms_new(void);
void atoms_free(struct atoms *);

struct atoms *atoms_load(struct save *);
void atoms_save(struct atoms *, struct save *);

bool atoms_set(struct atoms *, const struct symbol *, word_t id);
word_t atoms_atom(struct atoms *, const struct symbol *);
bool atoms_str(struct atoms *, word_t id, struct symbol *dst);
