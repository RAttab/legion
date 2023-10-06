/* types.h
   Remi Attab (remi.attab@gmail.com), 05 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef int64_t vm_word;
typedef uint8_t vm_reg;
typedef uint32_t vm_ip;

struct mod;
typedef uint32_t mod_id; // see mod.h for the full definition
