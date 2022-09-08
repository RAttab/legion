/* config.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "game/id.h"
#include "items/io.h"
#include "items/item.h"
#include "vm/vm.h"

#include "SDL.h"

struct flow;
struct font;
struct chunk;
struct atoms;
struct energy;
struct im_config;
struct ui_layout;


// -----------------------------------------------------------------------------
// fn
// -----------------------------------------------------------------------------

typedef void (*im_config_fn) (struct im_config *);

typedef void (*im_init_fn) (void *state, struct chunk *, id id);
typedef void (*im_make_fn) (
        void *state, struct chunk *, id id, const word *data, size_t len);
typedef void (*im_load_fn) (void *state, struct chunk *);
typedef void (*im_step_fn) (void *state, struct chunk *);
typedef void (*im_io_fn) (
        void *state, struct chunk *, enum io, id src, const word *args, size_t len);
typedef bool (*im_flow_fn) (const void *state, struct flow *);

typedef void *(*im_ui_alloc_fn)  (struct font *);
typedef void  (*im_ui_free_fn)   (void *state);
typedef void  (*im_ui_update_fn) (void *state, struct chunk *, id);
typedef bool  (*im_ui_event_fn)  (void *state, const SDL_Event *);
typedef void  (*im_ui_render_fn) (void *state, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// im_config
// -----------------------------------------------------------------------------

struct im_config
{
    enum item type;

    const char *str;
    size_t str_len;

    const char *atom;

    im_config_fn init;

    size_t size;

    uint8_t lab_bits;
    uint8_t lab_work;

    struct
    {
        im_init_fn init;
        im_make_fn make;
        im_load_fn load;
        im_step_fn step;
        im_io_fn io;
        im_flow_fn flow;
    } im;

    struct
    {
        im_ui_alloc_fn alloc;
        im_ui_free_fn free;
        im_ui_update_fn update;
        im_ui_event_fn event;
        im_ui_render_fn render;
    } ui;

    size_t io_list_len;
    const word *io_list;
};

const struct im_config *im_config(enum item);

inline const struct im_config *im_config_assert(enum item item)
{
    const struct im_config *config = im_config(item);
    assert(config);

    return config;
}

void im_populate(void);
void im_populate_atoms(struct atoms *);
