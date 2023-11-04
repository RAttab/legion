/* storage.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/storage/storage_im.c"
#include "items/storage/storage_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_storage_config(struct im_config *config)
{
    config->size = sizeof(struct im_storage);

    config->im.init = im_storage_init;
    config->im.step = im_storage_step;
    config->im.io = im_storage_io;
    config->im.flow = im_storage_flow;

    config->ui.alloc = ui_storage_alloc;
    config->ui.free = ui_storage_free;
    config->ui.update = ui_storage_update;
    config->ui.render = ui_storage_render;

    config->io.list = im_storage_io_list;
    config->io.len = array_len(im_storage_io_list);
}
