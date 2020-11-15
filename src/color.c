/* render.c
   Rémi Attab (remi.attab@gmail.com), 07 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "color.h"

// ref: https://stackoverflow.com/a/3407960
struct rgb spectrum_rgb(uint32_t spectrum, uint32_t max)
{
    double w = 380 + ((780 - 380) * ((double)spectrum) / max);
    
    if (w >= 380 && w < 440) {
        return (struct rgb) {
            .r = 255 * (-(w - 440) / (440 - 380)),
            .g = 0,
            .b = 255,
        };
    }
        
    if (w >= 440 && w < 490) {
        return (struct rgb) {
            .r = 0,
            .g = 255 * ((w - 440) / (490 - 440)),
            .b = 255,
        };
    }

    if (w >= 490 && w < 510) {
        return (struct rgb) {
            .r = 0,
            .g = 255,
            .b = 255 * (-(w - 510) / (510 - 490)),
        };
    }
        
    if (w >= 510 && w < 580) {
        return (struct rgb) {
            .r = 255 * ((w - 510) / (580 - 510)),
            .g = 255,
            .b = 0,
        };
    }

    if (w >= 580 && w < 645) {
        return (struct rgb) {
            .r = 255,
            .g = 255 * (-(w - 645) / (645 - 580)),
            .b = 0,
        };
    }

    if ( w >= 645 && w <= 780) {
        return (struct rgb) { .r = 255, .g = 0, .b = 0 };
    }

    return (struct rgb) {0};
}

// ref: https://stackoverflow.com/a/20820649
struct rgb desaturate(struct rgb old, double f)
{
    double L = 0.3*old.r + 0.6*old.g + 0.1*old.b;
    return (struct rgb) {
        .r = old.r + f * (L - old.r),
        .g = old.g + f * (L - old.g),
        .b = old.b + f * (L - old.b),
    };
}
