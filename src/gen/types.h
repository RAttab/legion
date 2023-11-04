/* types.h
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// im_type
// -----------------------------------------------------------------------------

enum im_type
{
    im_type_nil = 0,
    im_type_natural,
    im_type_synthetic,
    im_type_logistics,
    im_type_active,
    im_type_passive,
    im_type_sys,
};

// Required to be inlined so gen doesn't depend game
inline const char *im_type_str(enum im_type type)
{
    switch (type) {
    case im_type_nil:       { return "nil"; }
    case im_type_natural:   { return "natural"; }
    case im_type_synthetic: { return "synth"; }
    case im_type_passive:   { return "passive"; }
    case im_type_active:    { return "active"; }
    case im_type_logistics: { return "logistics"; }
    case im_type_sys:       { return "sys"; }
    default:                { return "???"; }
    }
}

inline bool im_type_elem(enum im_type type)
{
    switch (type) {
    case im_type_natural:
    case im_type_synthetic: return true;
    default: return false;
    }
}
