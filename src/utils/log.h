/* log.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include "SDL.h"

#include <errno.h>
#include <stdio.h>

// -----------------------------------------------------------------------------
// err
// -----------------------------------------------------------------------------

#define err0(msg)                                       \
    do {                                                \
        fprintf(stderr, "fail<%s:%u> " msg "\n",        \
                __FILE__, __LINE__);                    \
    } while(false)

#define err(fmt, ...)                                   \
    do {                                                \
        fprintf(stderr, "fail<%s:%u> " fmt "\n",        \
                __FILE__, __LINE__, __VA_ARGS__);       \
    } while(false)

#define err_errno(fmt, ...)                                             \
    do {                                                                \
        fprintf(stderr, "fail<%s:%u> " fmt ": %s (%d)\n",               \
                __FILE__, __LINE__, __VA_ARGS__, strerror(errno), errno); \
    } while(false)

#define err_posix(errnum, fmt, ...)                                     \
    do {                                                                \
        fprintf(stderr, "fail<%s:%u> " fmt ": %s (%d)\n",               \
                __FILE__, __LINE__, __VA_ARGS__, strerror(errnum), errno); \
    } while(false)

#define err_posix0(errnum, msg)                                 \
    do {                                                        \
        fprintf(stderr, "fail<%s:%u> " msg ": %s (%d)\n",       \
                __FILE__, __LINE__, strerror(errnum), errno);   \
    } while(false)



// -----------------------------------------------------------------------------
// fail
// -----------------------------------------------------------------------------

#define fail(fmt, ...)                          \
    do {                                        \
        err(fmt, __VA_ARGS__);                  \
        abort();                                \
    } while(false)

#define fail_errno(fmt, ...)                    \
    do {                                        \
        err(fmt, __VA_ARGS__);                  \
        abort();                                \
    } while(false)

// -----------------------------------------------------------------------------
// sdl
// -----------------------------------------------------------------------------

#define sdl_fail(p)                                                     \
    {                                                                   \
        fprintf(stderr, "sdl-fail<%s:%u> %s: %s\n",                     \
                __FILE__, __LINE__, #p, SDL_GetError());                \
        abort();                                                        \
    } while(false)


#define sdl_err(p)                                                      \
    ({                                                                  \
        typeof(p) ret = (p);                                            \
        if (unlikely(ret < 0)) sdl_fail(p);                             \
        ret;                                                            \
    })

#define sdl_ptr(p)                                                      \
    ({                                                                  \
        typeof(p) ret = (p);                                            \
        if (unlikely(!ret)) sdl_fail(p);                                \
        ret;                                                            \
    })

