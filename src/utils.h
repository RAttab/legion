/* utils.h
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2017
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "SDL.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <linux/limits.h>

// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

#define legion_packed       __attribute__((__packed__))
#define legion_noreturn     __attribute__((noreturn))
#define legion_printf(x,y)  __attribute__((format(printf, x, y)))
#define likely(x)    __builtin_expect(x, 1)
#define unlikely(x)  __builtin_expect(x, 0)


// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

#define array_len(arr) (sizeof((arr)) / sizeof((arr)[0]))


// -----------------------------------------------------------------------------
// logs
// -----------------------------------------------------------------------------

#define SDL_LogErrno(fmt, ...) \
    SDL_Log(fmt ": (%d) %s", __VA_ARGS__, errno, strerror(errno))

// -----------------------------------------------------------------------------
// vma
// -----------------------------------------------------------------------------

static const size_t page_len = 4096;

static inline size_t to_vma_len(size_t len)
{
    if (!(len % page_len)) return len;
    return (len & ~(page_len - 1)) + page_len;
}

// -----------------------------------------------------------------------------
// bits
// -----------------------------------------------------------------------------

inline size_t u32_clz(uint32_t x) { return x ? __builtin_clz(x) : 32; }

inline size_t u64_clz(uint64_t x) { return x ? __builtin_clzl(x) : 64; }
inline size_t u64_log2(uint64_t x) { return 63 - u64_clz(x); }

// -----------------------------------------------------------------------------
// math
// -----------------------------------------------------------------------------

inline uint32_t u32_min(uint32_t x, uint32_t y)
{
    return x <= y ? x : y;
}

inline int64_t i64_min(int64_t x, int64_t y)
{
    return x <= y ? x : y;
}

inline int64_t i64_max(int64_t x, int64_t y)
{
    return x >= y ? x : y;
}

inline int64_t i64_clamp(int64_t x, int64_t min, int64_t max)
{
    return i64_min(i64_max(x, min), max);
}

// -----------------------------------------------------------------------------
// vec64
// -----------------------------------------------------------------------------

struct vec64
{
    uint32_t len, cap;
    uint64_t vals[];
};

inline void vec64_free(struct vec64 *vec) { free(vec); }

inline struct vec64 *vec64_reserve(size_t size)
{
    struct vec64 *vec = calloc(1, sizeof(*vec) + size * sizeof(vec->items[0]));
    vec->cap = size;
    return vec;
}

inline struct vec64 *vec64_append(struct vec64 *vec, uint64_t val)
{
    if (unlikely(!vec || vec->len == vec->cap)) {
        size_t size = vec ? 1 : vec->cap * 2;
        vec = realloc(vec, sizeof(*vec) + size * sizeof(vec->items[0]));
    }
    vec->items[vec->len++] = val;
    return vec;
}

// -----------------------------------------------------------------------------
// sdl
// -----------------------------------------------------------------------------

#define sdl_fail(p)                                                     \
    {                                                                   \
        fprintf(stderr, "sdl-error<%s, %u> %s: %s\n",                   \
                __FILE__, __LINE__, #p, SDL_GetError());                \
        abort();                                                        \
    } while(false)


#define sdl_err(p)                                                      \
    ({                                                                  \
        typeof(p) ret = (p);                                            \
        if (unlikely(ret < 0)) sdl_fail();                              \
        ret;                                                            \
    })

#define sdl_ptr(p)                                                      \
    ({                                                                  \
        typeof(p) ret = (p);                                            \
        if (unlikely(!ret)) sdl_fail();                                 \
        ret;                                                            \
    })
