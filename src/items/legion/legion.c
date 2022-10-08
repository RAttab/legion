/* legion.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/legion/legion.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/legion/legion_im.c"
#include "items/legion/legion_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_legion_config(struct im_config *config)
{
    config->size = sizeof(struct im_legion);

    config->im.init = im_legion_init;
    config->im.make = im_legion_make;
    config->im.io = im_legion_io;

    config->ui.alloc = ui_legion_alloc;
    config->ui.free = ui_legion_free;
    config->ui.update = ui_legion_update;
    config->ui.event = ui_legion_event;
    config->ui.render = ui_legion_render;

    config->io.list = im_legion_io_list;
    config->io.len = array_len(im_legion_io_list);
}
