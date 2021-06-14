/* ui.h
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar;
struct ui_topbar *ui_topbar_new(void);
enum ui_ret ui_topbar_event(struct ui_topbar *, SDL_Event *);
void ui_topbar_render(struct ui_topbar *, SDL_Renderer *);
