/* nk.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define NK_MEMSET memset
#define NK_MEMCPY memcpy
#define NK_SQRT sqrt
#define NK_SIN sin
#define NK_COS cos
// #define NK_STRTOD strtod
// #define NK_DTOA dtoa
#define NK_VSNPRINTF vsnprintf

#define NK_INCLUDE_FIXED_TYPES
// #define NK_INCLUDE_DEFAULT_ALLOCATOR
// #define NK_INCLUDE_STANDARD_IO
// #define NK_INCLUDE_STANDARD_VARARGS
// #define NK_INCLUDE_FONT_BAKING

#include "nuklear.h"
