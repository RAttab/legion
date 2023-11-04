/* transmit.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/transmit/transmit_im.c"
#include "items/transmit/transmit_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_transmit_config(struct im_config *config)
{
    config->size = sizeof(struct im_transmit);

    config->im.init = im_transmit_init;
    config->im.step = im_transmit_step;
    config->im.io = im_transmit_io;

    config->ui.alloc = ui_transmit_alloc;
    config->ui.free = ui_transmit_free;
    config->ui.update = ui_transmit_update;
    config->ui.render = ui_transmit_render;

    config->io.list = im_transmit_io_list;
    config->io.len = array_len(im_transmit_io_list);
}
