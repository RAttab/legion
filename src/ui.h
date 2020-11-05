/* ui.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "SDL.h"

struct sector;
struct ui_core;

struct ui_core *ui_core_init(SDL_Renderer *, struct sector *, SDL_Rect);
void ui_core_free(struct ui_core *);

void ui_core_render(struct ui_core *, SDL_Renderer *);
void ui_core_events(struct ui_core *, SDL_Event *);

