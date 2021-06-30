/* utils.h
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2017
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include <math.h>
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
// macro shit
// -----------------------------------------------------------------------------

#define legion_concat_impl(x, y) x##y
#define legion_concat(x, y) legion_concat_impl(x, y)
#define legion_pad(n) uint8_t legion_concat(_pad_, __COUNTER__) [n]

#define legion_assert_type_eq(x, y)                             \
    static_assert(                                              \
            _Generic((x), typeof(y): true, default: false),     \
            "type mismatch: " #x " != " #y )

#define legion_max(x, y) ({                                             \
            typeof(x) __x = (x);                                        \
            typeof(y) __y = (y);                                        \
            __x >= __y ? __x : __y;                                     \
        })

#define legion_min(x, y) ({                                             \
            typeof(x) __x = (x);                                        \
            typeof(y) __y = (y);                                        \
            __x <= __y ? __x : __y;                                     \
        })


// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

enum { s_cache_line = 64 };
static const size_t cache_line = s_cache_line;

enum { s_page_len = 64 };
static const size_t page_len = s_page_len;


// -----------------------------------------------------------------------------
// alloc
// -----------------------------------------------------------------------------

inline void *alloc_cache(size_t n)
{
    assert(n % s_cache_line == 0);
    return memset(aligned_alloc(n, n), 0, n);
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

#define dbg(_fmt_, ...)                                                 \
    do {                                                                \
        fprintf(stderr, "%s:%u: " _fmt_ "\n", __FILE__, __LINE__, __VA_ARGS__); \
    } while (false)

