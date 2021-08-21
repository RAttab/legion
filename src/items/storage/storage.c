/* storage.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/storage/storage.h"
#include "items/config.h"


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

    config->gm.init = im_storage_init;
    config->gm.step = im_storage_step;
    config->gm.io = im_storage_io;
    config->gm.flow = im_storage_flow;

    config->ui.alloc = ui_storage_alloc;
    config->ui.free = ui_storage_free;
    config->ui.update = ui_storage_update;
    config->ui.render = ui_storage_render;

    config->io_list = im_storage_io_list;
    config->io_list_len = array_len(im_storage_io_list);
}
