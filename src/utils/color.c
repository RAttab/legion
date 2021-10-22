/* color.c
   Rémi Attab (remi.attab@gmail.com), 22 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/color.h"

#include <math.h>


// -----------------------------------------------------------------------------
// hsv
// -----------------------------------------------------------------------------

// Reference: https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
struct rgba hsv_to_rgb(struct hsv hsv)
{
    double c = hsv.v * hsv.s;
    double h = (hsv.h * 360) / 60;
    double x = c * (1 - fabs(fmod(h, 2) - 1));

    double r = 0, g = 0, b = 0;

    if (h < 1)      { r = c; g = x; b = 0; }
    else if (h < 2) { r = x; g = c; b = 0; }
    else if (h < 3) { r = 0; g = c; b = x; }
    else if (h < 4) { r = 0; g = x; b = c; }
    else if (h < 5) { r = x; g = 0; b = c; }
    else if (h < 6) { r = c; g = 0; b = x; }
    else assert(false);

    double m = hsv.v - c;
    return make_rgba(
            (r + m) * 255,
            (g + m) * 255,
            (b + m) * 255,
            255);
}

// Reference: https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB
struct hsv hsv_from_rgb(struct rgba rgb)
{
    double r = ((double) rgb.r) / 255;
    double g = ((double) rgb.g) / 255;
    double b = ((double) rgb.b) / 255;

    double min = legion_min(r, legion_min(g, b));
    double v = legion_max(r, legion_max(g, b));

    double h = 0;
    double c = v - min;
    if (c == 0) { h = 0; }
    else if (v == r) { h = 60 * (0 + ((g - b) / c)); }
    else if (v == g) { h = 60 * (2 + ((b - r) / c)); }
    else if (v == b) { h = 60 * (4 + ((r - g) / c)); }

    double s = v == 0 ? 0 : c / v;
    return (struct hsv) { .h = h / 360, .s = s, .v = v };
}
