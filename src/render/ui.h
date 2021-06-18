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
void ui_topbar_free(struct ui_topbar *);
int16_t ui_topbar_height(const struct ui_topbar *);
bool ui_topbar_event(struct ui_topbar *, SDL_Event *);
void ui_topbar_render(struct ui_topbar *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct ui_mods;
struct ui_mods *ui_mods_new(void);
void ui_mods_free(struct ui_mods *);
bool ui_mods_event(struct ui_mods *, SDL_Event *);
void ui_mods_render(struct ui_mods *, SDL_Renderer *);
int16_t ui_mods_width(const struct ui_mods *);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct ui_mod;
struct ui_mod *ui_mod_new(void);
void ui_mod_free(struct ui_mod *);
bool ui_mod_event(struct ui_mod *, SDL_Event *);
void ui_mod_render(struct ui_mod *, SDL_Renderer *);
