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

#ifndef __SIZEOF_INT128__
# error "I need my 128 bit ints"
#endif

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;


constexpr size_t sys_cache_line_len = 64;
constexpr size_t sys_page_len = 4096;


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
// mem
// -----------------------------------------------------------------------------

inline void mem_free(void *ptr) { free(ptr); }


#define mem_alloc_t(ptr) \
    ({ mem_alloc(sizeof(*ptr)); })

inline void *mem_alloc(size_t len)
{
    return calloc(1, len);
}

inline void *mem_realloc(void *ptr, size_t old, size_t new)
{
    ptr = realloc(ptr, new);
    if (likely(new > old)) memset(ptr + old, 0, new - old);
    return ptr;
}


#define mem_array_alloc_t(elem, count) \
    ({ mem_array_alloc(sizeof(elem), (count)); })
#define mem_array_realloc_t(ptr, old, new) \
    ({ mem_array_realloc(ptr, sizeof(*ptr), (old), (new)); })

#define mem_array_len_grow(_ptr, init)                  \
    ({                                                  \
        typeof(_ptr) ptr = (_ptr);                      \
        typeof(*ptr) old = *ptr;                        \
        *ptr = old ? old * 2 : (init);                  \
        old;                                            \
    })

inline void *mem_array_alloc(size_t elem, size_t count)
{
    return calloc(count, elem);
}

inline void *mem_array_realloc(void *ptr, size_t elem, size_t old, size_t new)
{
    ptr = reallocarray(ptr, new, elem);
    if (likely(new > old)) memset(ptr + (old * elem), 0, (new - old) * elem);
    return ptr;
}


#define mem_struct_alloc_t(ptr, elem, count) \
    ({ mem_struct_alloc(sizeof(*ptr), sizeof(elem), (count)); })
#define mem_struct_realloc_t(ptr, elem, old, new) \
    ({ mem_struct_realloc((ptr), sizeof(*ptr), sizeof(elem), (old), (new)); })

inline void *mem_struct_alloc(size_t head, size_t elem, size_t count)
{
    return calloc(1, head + (count * elem));
}

inline void *mem_struct_realloc(
        void *ptr, size_t head, size_t elem, size_t old, size_t new)
{
    ptr = realloc(ptr, head + (new * elem));
    if (likely(new > old))
        memset(ptr + head + (old * elem), 0, (new - old) * elem);
    return ptr;
}


#define mem_align_alloc_t(ptr, align) \
    ({ mem_align_alloc(sizeof(*ptr), (align)); })

inline void *mem_align_alloc(size_t len, size_t align)
{
    return memset(aligned_alloc(align, len), 0, len);
}

inline void *mem_align_realloc(void *ptr, size_t old, size_t new, size_t align)
{
    return memcpy(mem_align_alloc(new, align), ptr, old);
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


