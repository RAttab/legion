/* factory.h
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

struct factory;

struct factory *factory_new(void);
void factory_free(struct factory *);

void factory_render(struct factory *, SDL_Renderer *);
bool factory_event(struct factory *, SDL_Event *);
