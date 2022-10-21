/* config.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "game/id.h"
#include "db/io.h"
#include "db/items.h"
#include "vm/vm.h"

#include "SDL.h"

struct flow;
struct chunk;
struct atoms;
struct energy;
struct im_config;
struct ui_layout;


// -----------------------------------------------------------------------------
// fn
// -----------------------------------------------------------------------------

typedef void (*im_config_fn) (struct im_config *);

typedef void (*im_init_fn) (void *state, struct chunk *, im_id id);
typedef void (*im_make_fn) (
        void *state, struct chunk *, im_id id, const vm_word *data, size_t len);
typedef void (*im_load_fn) (void *state, struct chunk *);
typedef void (*im_step_fn) (void *state, struct chunk *);
typedef void (*im_io_fn) (
        void *state, struct chunk *, enum io, im_id src, const vm_word *args, size_t len);
typedef bool (*im_flow_fn) (const void *state, struct flow *);

typedef void *(*im_ui_alloc_fn)  (void);
typedef void  (*im_ui_free_fn)   (void *state);
typedef void  (*im_ui_update_fn) (void *state, struct chunk *, im_id);
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

    struct
    {
        size_t len;
        const struct io_cmd *list;
    } io;
};


void io_populate_atoms(struct atoms *atoms);
