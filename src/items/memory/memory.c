/* memory.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/memory/memory.h"
#include "items/config.h"

// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    im_memory_len_base = 7 + 8 * 0,
    im_memory_len_max = im_memory_len_base,
};

// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/memory/memory_im.c"
#include "items/memory/memory_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_memory_config(struct im_config *config)
{
    config->size = sizeof(struct im_memory);

    switch(config->type) {
    case item_memory: { config->size += im_memory_len_base * sizeof(vm_word); break; }
    default: { assert(false); }
    }

    config->im.init = im_memory_init;
    config->im.make = im_memory_make;
    config->im.io = im_memory_io;

    config->ui.alloc = ui_memory_alloc;
    config->ui.free = ui_memory_free;
    config->ui.update = ui_memory_update;
    config->ui.event = ui_memory_event;
    config->ui.render = ui_memory_render;

    config->io.list = im_memory_io_list;
    config->io.len = array_len(im_memory_io_list);
}
