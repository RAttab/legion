/* utils.h
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2017
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <linux/limits.h>


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

#define legion_packed       __attribute__((__packed__))
#define legion_printf(x,y)  __attribute__((format(printf, x, y)))

#define likely(x)    __builtin_expect(x, 1)
#define unlikely(x)  __builtin_expect(x, 0)


// -----------------------------------------------------------------------------
// arrays
// -----------------------------------------------------------------------------

#define array_len(arr) (sizeof((arr)) / sizeof((arr)[0]))


// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

static const size_t cache_line = 64;
static const size_t page_len = 4096;


// -----------------------------------------------------------------------------
// alloc
// -----------------------------------------------------------------------------

inline void *alloc_cache(size_t n)
{
    assert(n % cache_line == 0);
    return memset(aligned_alloc(n, cache_line), 0, n);
}
