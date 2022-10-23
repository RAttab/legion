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
coord_scale map_scale(struct map *);
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
coord_scale factory_scale(struct factory *);
struct coord factory_coord(struct factory *);


// -----------------------------------------------------------------------------
// topbar
// -----------------------------------------------------------------------------

struct ui_topbar;
struct ui_topbar *ui_topbar_new(void);
void ui_topbar_free(struct ui_topbar *);
int16_t ui_topbar_height(void);
bool ui_topbar_event(struct ui_topbar *, SDL_Event *);
void ui_topbar_render(struct ui_topbar *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

struct ui_status;
struct ui_status *ui_status_new(void);
void ui_status_free(struct ui_status *);
int16_t ui_status_height(void);
bool ui_status_event(struct ui_status *, SDL_Event *);
void ui_status_render(struct ui_status *, SDL_Renderer *);

void ui_status_set(struct ui_status *, enum status_type, const char *msg, size_t len);


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

struct ui_tapes;
struct ui_tapes *ui_tapes_new(void);
void ui_tapes_free(struct ui_tapes *);
bool ui_tapes_event(struct ui_tapes *, SDL_Event *);
void ui_tapes_render(struct ui_tapes *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct ui_mods;
struct ui_mods *ui_mods_new(void);
void ui_mods_free(struct ui_mods *);
bool ui_mods_event(struct ui_mods *, SDL_Event *);
void ui_mods_render(struct ui_mods *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

struct ui_stars;
struct ui_stars *ui_stars_new(void);
void ui_stars_free(struct ui_stars *);
bool ui_stars_event(struct ui_stars *, SDL_Event *);
void ui_stars_render(struct ui_stars *, SDL_Renderer *);

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


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct ui_worker;
struct ui_worker *ui_worker_new(void);
void ui_worker_free(struct ui_worker *);
void ui_worker_update_state(struct ui_worker *);
bool ui_worker_event(struct ui_worker *, SDL_Event *);
void ui_worker_render(struct ui_worker *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

struct ui_energy;
struct ui_energy *ui_energy_new(void);
void ui_energy_free(struct ui_energy *);
void ui_energy_update_state(struct ui_energy *);
bool ui_energy_event(struct ui_energy *, SDL_Event *);
void ui_energy_render(struct ui_energy *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

struct ui_log;
struct ui_log *ui_log_new(void);
void ui_log_free(struct ui_log *);
bool ui_log_event(struct ui_log *, SDL_Event *);
void ui_log_render(struct ui_log *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------
struct ui_man;
struct ui_man *ui_man_new(void);
void ui_man_free(struct ui_man *);
bool ui_man_event(struct ui_man *, SDL_Event *);
void ui_man_render(struct ui_man *, SDL_Renderer *);
