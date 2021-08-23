/* ui_tape.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "ui/ui.h"
#include "game/tape.h"
#include "render/font.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct ui_tape
{
    struct font *font;
    struct ui_label tape, tape_val;
    struct ui_label energy, energy_val;
    struct ui_scroll scroll;
    struct ui_label index, in, out;
};

void ui_tape_init(struct ui_tape *, struct font *);
void ui_tape_free(struct ui_tape *);
void ui_tape_update(struct ui_tape *, tape_packed_t);
bool ui_tape_event(struct ui_tape *, tape_packed_t, const SDL_Event *);
void ui_tape_render(
        struct ui_tape *, tape_packed_t, struct ui_layout *, SDL_Renderer *);
