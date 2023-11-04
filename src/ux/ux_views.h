/* ux_views.h
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


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
