/* ui_tape.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "ui/ui.h"
#include "game/tape.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct ui_tape
{
    struct ui_label tape, tape_val;
    struct ui_label energy, energy_val;
    struct ui_scroll scroll;
    struct ui_label index, in, work, out;
};

void ui_tape_init(struct ui_tape *);
void ui_tape_free(struct ui_tape *);
void ui_tape_update(struct ui_tape *, tape_packed);
void ui_tape_event(struct ui_tape *, tape_packed);
void ui_tape_render(struct ui_tape *, tape_packed, struct ui_layout *);
