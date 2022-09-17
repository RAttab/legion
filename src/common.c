/* common.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"


// -----------------------------------------------------------------------------
// alloc
// -----------------------------------------------------------------------------

extern void *alloc_cache(size_t n);
extern void *realloc_zero(void *ptr, size_t old, size_t new, size_t size);
