/* ui.h
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "game/coord.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// map
// -----------------------------------------------------------------------------

struct map;

struct map *map_new(void);
void map_free(struct map *);
void map_render(struct map *, SDL_Renderer *);
bool map_event(struct map *, SDL_Event *);

bool map_active(struct map *);
scale_t map_scale(struct map *);
struct coord map_coord(struct map *);


// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

struct factory;
struct factory *factory_new(void);
void factory_free(struct factory *);
void factory_render(struct factory *, SDL_Renderer *);
bool factory_event(struct factory *, SDL_Event *);

bool factory_active(struct factory *);
scale_t factory_scale(struct factory *);
struct coord factory_coord(struct factory *);


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


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ui_star;
struct ui_star *ui_star_new(void);
void ui_star_free(struct ui_star *);
bool ui_star_event(struct ui_star *, SDL_Event *);
void ui_star_render(struct ui_star *, SDL_Renderer *);
int16_t ui_star_width(const struct ui_star *);


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

struct ui_item;
struct ui_item *ui_item_new(void);
void ui_item_free(struct ui_item *);
bool ui_item_event(struct ui_item *, SDL_Event *);
void ui_item_render(struct ui_item *, SDL_Renderer *);
int16_t ui_item_width(struct ui_item *);


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

struct ui_io;
struct ui_io *ui_io_new(void);
void ui_io_free(struct ui_io *);
bool ui_io_event(struct ui_io *, SDL_Event *);
void ui_io_render(struct ui_io *, SDL_Renderer *);
