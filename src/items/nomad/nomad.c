/* nomad.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/nomad/nomad.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/nomad/nomad_im.c"
#include "items/nomad/nomad_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_nomad_config(struct im_config *config)
{
    config->size = sizeof(struct im_nomad);

    config->im.init = im_nomad_init;
    config->im.make = im_nomad_make;
    config->im.step = im_nomad_step;
    config->im.io = im_nomad_io;
    config->im.flow = im_nomad_flow;

    config->ui.alloc = ui_nomad_alloc;
    config->ui.free = ui_nomad_free;
    config->ui.update = ui_nomad_update;
    config->ui.event = ui_nomad_event;
    config->ui.render = ui_nomad_render;

    config->io_list = im_nomad_io_list;
    config->io_list_len = array_len(im_nomad_io_list);
}
