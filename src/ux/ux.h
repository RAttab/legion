/* ui.h
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
#include "game/types.h"
#include "db/io.h"


// -----------------------------------------------------------------------------
// ui
// -----------------------------------------------------------------------------

enum ux_slot : uint8_t
{
    ux_slot_nil = 0U,

    ux_slot_back      = 1U << 0,
    ux_slot_right     = 1U << 1,
    ux_slot_right_sub = 1U << 2,
    ux_slot_left      = 1U << 3,

    ux_slot_len = 4U,
};

const char *ux_slot_str(enum ux_slot);

enum ux_view : uint8_t
{
    ux_view_nil = 0,

    ux_view_topbar,
    ux_view_status,

    ux_view_map,
    ux_view_factory,

    ux_view_tapes,
    ux_view_stars,
    ux_view_mods,
    ux_view_log,

    ux_view_star,
    ux_view_item,
    ux_view_pills,
    ux_view_energy,
    ux_view_workers,

    ux_view_man,

    ux_view_len,
};

const char *ux_view_str(enum ux_view);


typedef void (*ux_state_fn) (void *);
typedef void (*ux_event_fn) (void *);
typedef void (*ux_render_fn) (void *, struct ui_layout *);

struct ux_view_state
{
    void *state;
    enum ux_view view, parent;
    enum ux_slot slots;
    struct ui_panel *panel;

    struct
    {
        ux_state_fn free, show, hide;
        ux_state_fn update_state, update_frame;
        ux_event_fn event;
        ux_render_fn render;
    } fn;
};

void ux_init(void);
void ux_free(void);

void *ux_state(enum ux_view);
enum ux_view ux_slot(enum ux_slot);
struct ui_panel *ux_cursor_panel(void);

void ux_update_state(void);
void ux_update_frame(void);
void ux_event(void);
void ux_render(void);

void ux_reset(void);
void ux_show(enum ux_view);
void ux_show_slot(enum ux_view, enum ux_slot);
void ux_hide(enum ux_view);
void ux_toggle(enum ux_view);

void ux_log_msg(enum status_type, const char *msg, size_t len);
void ux_logv(enum status_type, const char *fmt, va_list);
void ux_log(enum status_type, const char *fmt, ...) legion_printf(2, 3);


// -----------------------------------------------------------------------------
// back
// -----------------------------------------------------------------------------

struct ux_map;
void ux_map_alloc(struct ux_view_state *);
void ux_map_show(struct coord);
void ux_map_goto(struct coord);
coord_scale ux_map_scale(void);
struct coord ux_map_coord(void);

struct ux_factory;
void ux_factory_alloc(struct ux_view_state *);
void ux_factory_show(struct coord, im_id);
coord_scale ux_factory_scale(void);
struct coord ux_factory_coord(void);


// -----------------------------------------------------------------------------
// top/bottom
// -----------------------------------------------------------------------------

struct ux_topbar;
void ux_topbar_alloc(struct ux_view_state *);

struct ux_status;
void ux_status_alloc(struct ux_view_state *);
void ux_status_set(enum status_type, const char *msg, size_t len);


// -----------------------------------------------------------------------------
// left
// -----------------------------------------------------------------------------

struct ux_tapes;
void ux_tapes_alloc(struct ux_view_state *);
void ux_tapes_show(enum item);

struct ux_mods;
void ux_mods_alloc(struct ux_view_state *);
void ux_mods_show(mod_id, vm_ip);
void ux_mods_debug(mod_id, bool debug, vm_ip ip, vm_ip bp);

struct ux_stars;
void ux_stars_alloc(struct ux_view_state *);

struct ux_log;
void ux_log_alloc(struct ux_view_state *);
void ux_log_show(struct coord);


// -----------------------------------------------------------------------------
// right
// -----------------------------------------------------------------------------

struct ux_star;
void ux_star_alloc(struct ux_view_state *);
void ux_star_show(struct coord);

struct ux_item;
void ux_item_alloc(struct ux_view_state *);
im_id ux_item_selected(void);
void ux_item_show(im_id id, struct coord star);
bool ux_item_io(enum io, enum item, const vm_word *, size_t);

struct ux_io;
struct ux_io *ux_io_alloc(void);
int16_t ux_io_width(void);
void ux_io_show(struct ux_io *, struct coord, im_id);
void ux_io_free(struct ux_io *);
void ux_io_event(struct ux_io *);
void ux_io_render(struct ux_io *, struct ui_layout *);

struct ux_pills;
void ux_pills_alloc(struct ux_view_state *);
void ux_pills_show(struct coord);

struct ux_workers;
void ux_workers_alloc(struct ux_view_state *);
void ux_workers_show(struct coord);

struct ux_energy;
void ux_energy_alloc(struct ux_view_state *);
void ux_energy_show(struct coord);


// -----------------------------------------------------------------------------
// float
// -----------------------------------------------------------------------------

struct ux_man;
void ux_man_alloc(struct ux_view_state *);

void ux_man_show(struct link);
void ux_man_show_slot(enum ux_slot, struct link);

void ux_man_show_path(const char *path, ...) legion_printf(1, 2);
void ux_man_show_slot_path(enum ux_slot, const char *path, ...) legion_printf(2, 3);
