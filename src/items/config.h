/* config.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "items/item.h"

// -----------------------------------------------------------------------------
// im_config
// -----------------------------------------------------------------------------

typedef void (*im_config_t) (struct im_config *);

typedef void (*im_gm_init_t) (
        void *state, struct chunk *, id_t id, const word_t *data, size_t len);
typedef void (*im_gm_load_t) (void *state, struct chunk *);
typedef void (*im_gm_step_t) (void *state, struct chunk *);
typedef void (*im_gm_io_t) (
        void *state, struct chunk *, enum io, id_t src, const word_t *args, size_t len);

typedef void *(*im_ui_alloc_t)  (void);
typedef void  (*im_ui_free_t)   (void *state);
typedef void  (*im_ui_update_t) (void *state, struct chunk *, id_t);
typedef void  (*im_ui_event_t)  (void *state, SDL_Event *);
typedef void  (*im_ui_render_t) (void *state, struct ui_layout *, SDL_Renderer *);

struct im_config
{
    enum item type;

    const char *str;
    size_t str_len;

    const char *atom;

    im_config_t init;

    size_t size;
    size_t travel;

    struct
    {
        im_gm_init_t init;
        im_gm_load_t load;
        im_gm_step_t step;
        im_gm_io_t io;
    } gm;

    struct
    {
        im_ui_alloc_t alloc;
        im_ui_free_t free;
        im_ui_update_t update;
        im_ui_event_t event;
        im_ui_render_t render;
    } ui;

    size_t io_list_len;
    const word_t *io_list;
};

const struct im_config *im_config(enum item);
void im_populate(struct atoms *);
