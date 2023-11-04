/* lab.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/lab/lab_im.c"
#include "items/lab/lab_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_lab_config(struct im_config *config)
{
    config->size = sizeof(struct im_lab);

    config->im.init = im_lab_init;
    config->im.step = im_lab_step;
    config->im.io = im_lab_io;
    config->im.flow = im_lab_flow;

    config->ui.alloc = ui_lab_alloc;
    config->ui.free = ui_lab_free;
    config->ui.update = ui_lab_update;
    config->ui.render = ui_lab_render;

    config->io.list = im_lab_io_list;
    config->io.len = array_len(im_lab_io_list);
}
