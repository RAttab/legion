/* color.h
   Rémi Attab (remi.attab@gmail.com), 22 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/log.h"
#include "SDL.h"

// -----------------------------------------------------------------------------
// rgba
// -----------------------------------------------------------------------------

struct rgba { uint8_t r, g, b, a; };
inline struct rgba make_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (struct rgba) { .r = r, .g = g, .b = b, .a = a };
}

inline struct rgba rgba_nil(void)       { return make_rgba(0x00, 0x00, 0x00, 0x00); }
inline struct rgba rgba_gray_a(uint8_t v, uint8_t a) { return make_rgba(v, v, v, a); }
inline struct rgba rgba_gray(uint8_t v) { return rgba_gray_a(v, 0xFF); }
inline struct rgba rgba_white(void )    { return rgba_gray(0xFF); }
inline struct rgba rgba_black(void )    { return rgba_gray(0x00); }
inline struct rgba rgba_red(void)       { return make_rgba(0xCC, 0x00, 0x00, 0xFF); }
inline struct rgba rgba_green(void)     { return make_rgba(0x00, 0xCC, 0x00, 0xFF); }
inline struct rgba rgba_blue(void)      { return make_rgba(0x00, 0x00, 0xCC, 0xFF); }
inline struct rgba rgba_yellow(void)    { return make_rgba(0xCC, 0xCC, 0x00, 0xFF); }

inline void rgba_render(struct rgba c, SDL_Renderer *renderer)
{
    sdl_err(SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a));
    sdl_err(SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND));
};


// -----------------------------------------------------------------------------
// hsv
// -----------------------------------------------------------------------------

struct hsv { float h, s, v; };

struct hsv hsv_from_rgb(struct rgba);
struct rgba hsv_to_rgb(struct hsv);
