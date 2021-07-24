/* vm.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.c"
#include "vm/mod.c"
#include "vm/lisp.c"
#include "vm/atoms.c"


// -----------------------------------------------------------------------------
// symbol
// -----------------------------------------------------------------------------
// these tend to sometimes not get inlined so we provide a backup version here.

extern inline struct symbol make_symbol(const char *str);
extern inline struct symbol make_symbol_len(const char *str, size_t len);
extern inline uint64_t symbol_hash(const struct symbol *symbol);
extern inline void symbol_normalize(struct symbol *symbol);

