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


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

#define legion_packed       __attribute__((__packed__))
#define legion_noreturn     __attribute__((noreturn))
#define legion_printf(x,y)  __attribute__((format(printf, x, y)))
#define legion_likely(x)    __builtin_expect(x, 1)
#define legion_unlikely(x)  __builtin_expect(x, 0)


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

inline size_t clz(uint64_t x) { return x ? __builtin_clzll(x) : 64; }

inline uint64_t leading_bit(uint64_t x)
{
    return x & (1ULL << (63 - clz(x)));
}
