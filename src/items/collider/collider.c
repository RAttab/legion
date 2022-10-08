/* collider.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/collider/collider.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/collider/collider_im.c"
#include "items/collider/collider_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_collider_config(struct im_config *config)
{
    config->size = sizeof(struct im_collider);

    config->im.init = im_collider_init;
    config->im.load = im_collider_load;
    config->im.step = im_collider_step;
    config->im.io = im_collider_io;
    config->im.flow = im_collider_flow;

    config->ui.alloc = ui_collider_alloc;
    config->ui.free = ui_collider_free;
    config->ui.update = ui_collider_update;
    config->ui.event = ui_collider_event;
    config->ui.render = ui_collider_render;

    config->io.list = im_collider_io_list;
    config->io.len = array_len(im_collider_io_list);
}
