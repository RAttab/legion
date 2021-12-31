/* symbol.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/symbol.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// symbol
// -----------------------------------------------------------------------------

extern inline uint64_t symbol_hash(const struct symbol *);
extern inline int symbol_cmp(const struct symbol *lhs, const struct symbol *rhs);
extern inline bool symbol_eq(const struct symbol *lhs, const struct symbol *rhs);


static void symbol_normalize(struct symbol *symbol)
{
    while (!symbol->c[symbol->len-1]) symbol->len--;
    memset(symbol->c + symbol->len, 0, symbol_cap - symbol->len);
}

struct symbol make_symbol(const char *str)
{
    struct symbol symbol = { .len = strnlen(str, symbol_cap), .zero = 0 };
    memcpy(symbol.c, str, symbol.len);
    symbol_normalize(&symbol);
    return symbol;
}

struct symbol make_symbol_len(const char *str, size_t len)
{
    struct symbol symbol = { .len = len, .zero = 0 };
    memcpy(symbol.c, str, symbol.len);
    symbol_normalize(&symbol);
    return symbol;
}


size_t symbol_parse(const char *it, size_t len, struct symbol *value)
{
    assert(value);

    const char *end = it + len;
    it += str_skip_spaces(it, end - it);

    const char *first = it;
    while (it < end && symbol_char(*it)) it++;
    if (it - first > symbol_cap) return false;

    *value = make_symbol_len(first, it - first);
    return true;
}

void symbol_save(const struct symbol *sym, struct save *save)
{
    save_write_magic(save, save_magic_symbol);
    save_write_value(save, sym->len);
    save_write(save, sym->c, sym->len);
    save_write_magic(save, save_magic_symbol);
}

bool symbol_load(struct symbol *sym, struct save *save)
{
    if (!save_read_magic(save, save_magic_symbol)) return false;
    save_read_into(save, &sym->len);
    save_read(save, sym->c, sym->len);
    return save_read_magic(save, save_magic_symbol);
}
