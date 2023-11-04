/* packer.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/packer/packer_im.c"
#include "items/packer/packer_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_packer_config(struct im_config *config)
{
    config->size = sizeof(struct im_packer);

    config->im.init = im_packer_init;
    config->im.step = im_packer_step;
    config->im.io = im_packer_io;
    config->im.flow = im_packer_flow;

    config->ui.alloc = ui_packer_alloc;
    config->ui.free = ui_packer_free;
    config->ui.update = ui_packer_update;
    config->ui.render = ui_packer_render;

    config->io.list = im_packer_io_list;
    config->io.len = array_len(im_packer_io_list);
}
