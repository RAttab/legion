/* im_type.h
   Rémi Attab (remi.attab@gmail.com), 25 Feb 2023
   FreeBSD-style copyright and disclaimer apply

   This enum is unused in context where we don't want to include items.h
   (e.g. code generator for items).
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// im_type
// -----------------------------------------------------------------------------

enum im_type
{
    im_type_nil = 0,

    im_type_natural,
    im_type_synthetic,
    im_type_passive,
    im_type_active,
    im_type_logistics,
    im_type_sys,

    im_type_max,
};

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
    default:                { return "unknown"; }
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
