/* str.h
   Rémi Attab (remi.attab@gmail.com), 09 Dec 2020
   FreeBSD-style copyright and disclaimer apply

   \todo need to write meself a proper string library
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

size_t str_utoa(uint64_t val, char *dst, size_t len);
