/* ui.h
   Rémi Attab (remi.attab@gmail.com), 14 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

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
        ux_state_fn update;
        ux_event_fn event;
        ux_render_fn render;
    } fn;
};

void ux_init(void);
void ux_free(void);

void *ux_state(enum ux_view);
enum ux_view ux_slot(enum ux_slot);
struct ui_panel *ux_cursor_panel(void);

void ux_update(void);
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
