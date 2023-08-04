/* ui.h
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "game/coord.h"
#include "game/protocol.h"
#include "game/proxy.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

enum ui_slot : uint8_t
{
    ui_slot_nil = 0U,

    ui_slot_back      = 1U << 0,
    ui_slot_right     = 1U << 1,
    ui_slot_right_sub = 1U << 2,
    ui_slot_left      = 1U << 3,

    ui_slot_len = 4U,
};

const char *ui_slot_str(enum ui_slot);

enum ui_view : uint8_t
{
    ui_view_nil = 0,

    ui_view_topbar,
    ui_view_status,

    ui_view_map,
    ui_view_factory,

    ui_view_tapes,
    ui_view_stars,
    ui_view_mods,
    ui_view_log,

    ui_view_star,
    ui_view_item,
    ui_view_pills,
    ui_view_energy,
    ui_view_workers,

    ui_view_man,

    ui_view_len,
};

const char *ui_view_str(enum ui_view);


typedef void (*ui_state_fn) (void *);
typedef void (*ui_update_fn) (void *, struct proxy *);
typedef bool (*ui_event_fn) (void *, SDL_Event *);
typedef void (*ui_render_fn) (void *, struct ui_layout *, SDL_Renderer *);

struct ui_view_state
{
    void *state;
    enum ui_view view, parent;
    enum ui_slot slots;
    struct ui_panel *panel;

    struct
    {
        ui_state_fn free, show, hide;
        ui_update_fn update_state;
        ui_update_fn update_frame;
        ui_event_fn event;
        ui_render_fn render;
    } fn;
};

void ui_init(void);
void ui_free(void);

void *ui_state(enum ui_view);
enum ui_view ui_slot(enum ui_slot);

void ui_update_state(struct proxy *);
void ui_update_frame(struct proxy *);
void ui_event(SDL_Event *);
void ui_render(SDL_Renderer *);

void ui_reset(void);
void ui_show(enum ui_view);
void ui_show_slot(enum ui_view, enum ui_slot);
void ui_hide(enum ui_view);
void ui_toggle(enum ui_view);


// -----------------------------------------------------------------------------
// cursor
// -----------------------------------------------------------------------------

void ui_cursor_init(void);
void ui_cursor_free(void);

size_t ui_cursor_size(void);
struct pos ui_cursor_pos(void);
SDL_Point ui_cursor_point(void);
bool ui_cursor_in(const SDL_Rect* rect);

void ui_cursor_update(void);
void ui_cursor_event(SDL_Event *);
void ui_cursor_render(SDL_Renderer *);


// -----------------------------------------------------------------------------
// back
// -----------------------------------------------------------------------------

struct ui_map;
void ui_map_alloc(struct ui_view_state *);
void ui_map_show(struct coord);
void ui_map_goto(struct coord);
coord_scale ui_map_scale(void);
struct coord ui_map_coord(void);

struct ui_factory;
void ui_factory_alloc(struct ui_view_state *);
void ui_factory_show(struct coord, im_id);
coord_scale ui_factory_scale(void);
struct coord ui_factory_coord(void);


// -----------------------------------------------------------------------------
// top/bottom
// -----------------------------------------------------------------------------

struct ui_topbar;
void ui_topbar_alloc(struct ui_view_state *);

struct ui_status;
void ui_status_alloc(struct ui_view_state *);
void ui_status_set(enum status_type, const char *msg, size_t len);


// -----------------------------------------------------------------------------
// left
// -----------------------------------------------------------------------------

struct ui_tapes;
void ui_tapes_alloc(struct ui_view_state *);
void ui_tapes_show(enum item);

struct ui_mods;
void ui_mods_alloc(struct ui_view_state *);
void ui_mods_show(mod_id, vm_ip);
void ui_mods_breakpoint(mod_id, vm_ip);

struct ui_stars;
void ui_stars_alloc(struct ui_view_state *);

struct ui_log;
void ui_log_alloc(struct ui_view_state *);
void ui_log_show(struct coord);


// -----------------------------------------------------------------------------
// right
// -----------------------------------------------------------------------------

struct ui_star;
void ui_star_alloc(struct ui_view_state *);
void ui_star_show(struct coord);

struct ui_item;
void ui_item_alloc(struct ui_view_state *);
void ui_item_show(im_id id, struct coord star);
bool ui_item_io(enum io, enum item, const vm_word *, size_t);

struct ui_io;
struct ui_io *ui_io_alloc(void);
int16_t ui_io_width(void);
void ui_io_show(struct ui_io *, struct coord, im_id);
void ui_io_free(struct ui_io *);
bool ui_io_event(struct ui_io *, SDL_Event *);
void ui_io_render(struct ui_io *, struct ui_layout *, SDL_Renderer *);

struct ui_pills;
void ui_pills_alloc(struct ui_view_state *);
void ui_pills_show(struct coord);

struct ui_workers;
void ui_workers_alloc(struct ui_view_state *);
void ui_workers_show(struct coord);

struct ui_energy;
void ui_energy_alloc(struct ui_view_state *);
void ui_energy_show(struct coord);


// -----------------------------------------------------------------------------
// float
// -----------------------------------------------------------------------------

struct ui_man;
void ui_man_alloc(struct ui_view_state *);
void ui_man_show(struct link);
void ui_man_show_slot(struct link, enum ui_slot);
