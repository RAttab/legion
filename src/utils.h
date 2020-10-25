/* utils.h
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2017
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>


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
// err
// -----------------------------------------------------------------------------

void legion_abort() legion_noreturn;
void legion_exit(int code) legion_noreturn;

void legion_vfail(const char *file, int line, const char *fmt, ...)
    legion_printf(3, 4);

void legion_vfail_errno(const char *file, int line, const char *fmt, ...)
    legion_printf(3, 4);

#define legion_fail(...)                                \
    legion_vfail(__FILE__, __LINE__, __VA_ARGS__)

#define legion_fail_errno(...)                          \
    legion_vfail_errno(__FILE__, __LINE__, __VA_ARGS__)


// -----------------------------------------------------------------------------
// vma
// -----------------------------------------------------------------------------

static const size_t page_len = 4096;

static inline size_t to_vma_len(size_t len)
{
    if (!(len % page_len)) return len;
    return (len & ~(page_len - 1)) + page_len;
}
