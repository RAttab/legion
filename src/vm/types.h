/* types.h
   Remi Attab (remi.attab@gmail.com), 05 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// vm
// -----------------------------------------------------------------------------

typedef int64_t vm_word;
typedef uint8_t vm_reg;
typedef uint32_t vm_ip;


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mod;
struct mods;

typedef uint32_t mod_id;
typedef uint16_t mod_maj;
typedef uint16_t mod_ver;

inline mod_id make_mod(mod_maj maj, mod_ver ver)
{
    assert(!(maj >> 15));
    return maj << 16 | ver;
}

inline mod_maj mod_major(mod_id mod) { return mod >> 16; }
inline mod_ver mod_version(mod_id mod) { return ((1 << 16) - 1) & mod; }
inline bool mod_validate(vm_word word) { return word > 0 && word <= UINT32_MAX; }


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

enum : vm_word {
    atom_nil = 0x00,

    atom_ns_io   = 0x01000000,
    atom_ns_ioe  = 0x02000000,
    atom_ns_user = 0x80000000,
};
