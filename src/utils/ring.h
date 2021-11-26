/* ring.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply

   \todo Could be improved alot
*/

#pragma once


// -----------------------------------------------------------------------------
// ring32
// -----------------------------------------------------------------------------

#define ringx_type uint32_t
#define ringx_name ring32
#include "utils/ringx.h"


// -----------------------------------------------------------------------------
// ring64
// -----------------------------------------------------------------------------

#define ringx_type uint64_t
#define ringx_name ring64
#include "utils/ringx.h"
