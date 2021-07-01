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
// fail
// -----------------------------------------------------------------------------

#define fail(fmt, ...) \
    do {                                                                \
        fprintf(stderr, "fail<%s:%u> " fmt "\n",                        \
                __FILE__, __LINE__, __VA_ARGS__);                       \
        abort();                                                        \
    } while(false)

#define fail_errno(fmt, ...)                                            \
    do {                                                                \
        fprintf(stderr, "fail<%s:%u> " fmt ": %s\n",                    \
                __FILE__, __LINE__, __VA_ARGS__, strerror(errno));      \
        abort();                                                        \
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

