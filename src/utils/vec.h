/* vec.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// vec
// -----------------------------------------------------------------------------

#define vecx_type uint16_t
#define vecx_name vec16
#define vecx_eq
#define vecx_sort
#define vecx_sort_fn
#include "utils/vecx.h"

#define vecx_type uint32_t
#define vecx_name vec32
#define vecx_eq
#define vecx_sort
#define vecx_sort_fn
#include "utils/vecx.h"

#define vecx_type uint64_t
#define vecx_name vec64
#define vecx_sort_fn
#define vecx_eq
#define vecx_sort
#define vecx_sort_fn
#include "utils/vecx.h"
