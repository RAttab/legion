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

