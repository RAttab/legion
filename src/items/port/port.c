/* port.c
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "items/port/port.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/port/port_im.c"
#include "items/port/port_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_port_config(struct im_config *config)
{
    config->size = sizeof(struct im_port);

    config->im.init = im_port_init;
    config->im.step = im_port_step;
    config->im.io = im_port_io;
    config->im.flow = im_port_flow;

    config->ui.alloc = ui_port_alloc;
    config->ui.free = ui_port_free;
    config->ui.update = ui_port_update;
    config->ui.event = ui_port_event;
    config->ui.render = ui_port_render;

    config->io.list = im_port_io_list;
    config->io.len = array_len(im_port_io_list);
}
