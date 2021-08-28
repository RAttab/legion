/* receive.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/receive/receive.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    im_receive_buffer_max = 1,
};


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/receive/receive_im.c"
#include "items/receive/receive_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_receive_config(struct im_config *config)
{
    config->size =
        sizeof(struct im_receive) +
        sizeof(struct im_packet) * im_receive_buffer_max;

    config->im.init = im_receive_init;
    config->im.io = im_receive_io;

    config->ui.alloc = ui_receive_alloc;
    config->ui.free = ui_receive_free;
    config->ui.update = ui_receive_update;
    config->ui.render = ui_receive_render;

    config->io_list = im_receive_io_list;
    config->io_list_len = array_len(im_receive_io_list);
}
