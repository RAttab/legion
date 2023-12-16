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
#include <limits.h>
#include <linux/limits.h>


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

#define legion_unused        __attribute__((__unused__))
#define legion_packed        __attribute__((__packed__))
#define legion_aligned(x)    __attribute__((__aligned__ (x)))
#define legion_printf(x,y)   __attribute__((format(printf, x, y)))
#define legion_always_inline __attribute__((always_inline))

#define likely(x)    __builtin_expect(x, 1)
#define unlikely(x)  __builtin_expect(x, 0)

#define legion_atomic _Atomic
typedef legion_atomic float atomic_float;
typedef legion_atomic double atomic_double;


// -----------------------------------------------------------------------------
// macro utils
// -----------------------------------------------------------------------------

#define array_len(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define legion_concat_impl(x, y) x##y
#define legion_concat(x, y) legion_concat_impl(x, y)
#define legion_pad(n) uint8_t legion_concat(_pad_, __COUNTER__) [n]

#define legion_assert_type_eq(x, y)                             \
    static_assert(                                              \
            _Generic((x), typeof(y): true, default: false),     \
            "type mismatch: " #x " != " #y )

#define legion_max(x, y)                        \
    ({                                          \
        typeof(x) __x = (x);                    \
        typeof(y) __y = (y);                    \
        __x >= __y ? __x : __y;                 \
    })

#define legion_min(x, y)                        \
    ({                                          \
        typeof(x) __x = (x);                    \
        typeof(y) __y = (y);                    \
        __x <= __y ? __x : __y;                 \
    })

#define legion_bound(_val, _lo, _hi)            \
    ({                                          \
        typeof(_val) val = (_val);              \
        typeof(_lo) lo = (_lo);                 \
        typeof(_hi) hi = (_hi);                 \
        val < lo ? lo : (val > hi ? hi : val);  \
    })

#define legion_xchg(_ptr, _val)                 \
    ({                                          \
        typeof(_val) val = (_val);              \
        typeof(_ptr) ptr = (_ptr);              \
                                                \
        typeof(val) tmp = *ptr;                 \
        *ptr = val;                             \
        tmp;                                    \
    })

#define legion_swap(p0, _p1)                    \
    do {                                        \
        typeof(_p1) p1 = (_p1);                 \
        *p1 = legion_xchg(p0, *p1);             \
    } while(false)

#define legion_zero_from(ptr, field)            \
    do {                                        \
        typeof(ptr) tp = (ptr);                 \
        void *start = &(tp->field);             \
        void *end = tp + 1;                     \
        memset(start, 0, end - start);          \
    } while (false);

#define legion_zero_after(ptr, field)           \
    do {                                        \
        typeof(ptr) tp = (ptr);                 \
        void *start = (&(tp->field)) + 1;       \
        void *end = tp + 1;                     \
        memset(start, 0, end - start);          \
    } while (false);


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

#ifndef __SIZEOF_INT128__
# error "I need my 128 bit ints"
#endif

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

enum : size_t { s_cache_line = 64 };
static const size_t cache_line = s_cache_line;

enum : size_t { s_page_len = 4096 };
static const size_t page_len = s_page_len;


// -----------------------------------------------------------------------------
// alloc
// -----------------------------------------------------------------------------

inline void *alloc_cache(size_t len)
{
    return memset(aligned_alloc(s_cache_line, len), 0, len);
}

inline void *realloc_zero(void *ptr, size_t old, size_t new, size_t size)
{
    ptr = realloc(ptr, new * size);
    memset(ptr + (old * size), 0, (new - old) * size);
    return ptr;
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

#define dbg(_str_)                                                      \
    do {                                                                \
        fprintf(stderr, "%s:%u: " _str_ "\n", __FILE__, __LINE__);      \
    } while (false)

#define dbgf(_fmt_, ...)                                                \
    do {                                                                \
        fprintf(stderr, "%s:%u: " _fmt_ "\n", __FILE__, __LINE__, __VA_ARGS__); \
    } while (false)


