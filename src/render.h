/* render.h
   Rémi Attab (remi.attab@gmail.com), 07 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

// -----------------------------------------------------------------------------
// colors
// -----------------------------------------------------------------------------

struct rgb { uint8_t r, g, b; };

struct rgb spectrum_rgb(uint32_t spectrum, uint32_t max);
struct rgb desaturate(struct rgb, double percent);
