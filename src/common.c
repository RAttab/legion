/* common.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"


// -----------------------------------------------------------------------------
// alloc
// -----------------------------------------------------------------------------

extern void mem_free(void *);
extern void *mem_alloc(size_t);
extern void *mem_realloc(void *, size_t, size_t);
extern void *mem_array_alloc(size_t, size_t);
extern void *mem_array_realloc(void *, size_t, size_t, size_t);
extern void *mem_struct_alloc(size_t, size_t, size_t);
extern void *mem_struct_realloc(void *, size_t, size_t, size_t, size_t);
extern void *mem_align_alloc(size_t, size_t);
extern void *mem_align_realloc(void *, size_t, size_t, size_t);
