/* map.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "SDL.h"

// -----------------------------------------------------------------------------
// map
// -----------------------------------------------------------------------------

struct map;

struct map *map_new();
void map_free(struct map *);

scale_t map_scale(struct map *);

struct coord map_project_coord(struct map *, SDL_Point);
struct rect map_project_coord_rect(struct map *, const SDL_Rect *);
SDL_Point map_project_sdl(struct map *, struct coord);

void map_render(struct map *, SDL_Renderer *);
bool map_event(struct map *, SDL_Event *);
