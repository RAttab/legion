/* events.h
   Remi Attab (remi.attab@gmail.com), 05 Oct 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// events - io
// -----------------------------------------------------------------------------

enum ev_state : uint8_t
{
    ev_state_nil = 0,
    ev_state_up,
    ev_state_down,
    ev_state_repeat,
};

enum ev_mods : uint8_t
{
    ev_mods_nil = 0,
    ev_mods_alt = 1U << 0,
    ev_mods_ctrl = 1U << 1,
    ev_mods_shift = 1U << 2,
    ev_mods_caps_lock = 1U << 3,
};

inline bool ev_mods_allows(enum ev_mods mods, enum ev_mods mask)
{
    return !(mods & ~mask);
}

constexpr uint16_t ev_key_escape = GLFW_KEY_ESCAPE;
constexpr uint16_t ev_key_delete = GLFW_KEY_DELETE;
constexpr uint16_t ev_key_return = GLFW_KEY_ENTER;
constexpr uint16_t ev_key_tab = GLFW_KEY_TAB;
constexpr uint16_t ev_key_space = GLFW_KEY_SPACE;
constexpr uint16_t ev_key_backspace = GLFW_KEY_BACKSPACE;
constexpr uint16_t ev_key_left = GLFW_KEY_LEFT;
constexpr uint16_t ev_key_right = GLFW_KEY_RIGHT;
constexpr uint16_t ev_key_up = GLFW_KEY_UP;
constexpr uint16_t ev_key_down = GLFW_KEY_DOWN;
constexpr uint16_t ev_key_page_up = GLFW_KEY_PAGE_UP;
constexpr uint16_t ev_key_page_down = GLFW_KEY_PAGE_DOWN;
constexpr uint16_t ev_key_home = GLFW_KEY_HOME;
constexpr uint16_t ev_key_end = GLFW_KEY_END;

struct ev_key { enum ev_state state; enum ev_mods mods; uint16_t c; };
const struct ev_key *ev_next_key(const struct ev_key *);
void ev_consume_key(const struct ev_key *);

constexpr uint8_t ev_button_left = GLFW_MOUSE_BUTTON_LEFT;
constexpr uint8_t ev_button_right = GLFW_MOUSE_BUTTON_RIGHT;
constexpr uint8_t ev_button_middle = GLFW_MOUSE_BUTTON_MIDDLE;
struct ev_button { enum ev_state state; enum ev_mods mods; uint8_t button; };
const struct ev_button *ev_next_button(const struct ev_button *);
void ev_consume_button(const struct ev_button *);
enum ev_state ev_button_state(uint8_t button);
bool ev_button_down(uint8_t button);

struct ev_scroll { unit dx, dy; };
const struct ev_scroll *ev_next_scroll(const struct ev_scroll *);
void ev_consume_scroll(const struct ev_scroll *);

struct ev_mouse { struct pos p, d; };
const struct ev_mouse *ev_mouse(void);
struct pos ev_mouse_pos(void);
bool ev_mouse_in(struct rect);


// -----------------------------------------------------------------------------
// events - legion
// -----------------------------------------------------------------------------

struct ev_load {};
const struct ev_load *ev_load(void);
void ev_set_load(void);

struct ev_select_star { struct coord star; };
const struct ev_select_star *ev_select_star(void);
void ev_set_select_star(struct coord star);

struct ev_select_item { struct coord star; im_id item; };
const struct ev_select_item *ev_select_item(void);
void ev_set_select_item(struct coord star, im_id item);

struct ev_mod_break { mod_id mod; vm_ip ip; };
const struct ev_mod_break *ev_mod_break(void);
void ev_set_mod_break(mod_id mod, vm_ip ip);
