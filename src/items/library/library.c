/* library.c
   Rémi Attab (remi.attab@gmail.com), 28 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/library/library_im.c"
#include "items/library/library_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_library_config(struct im_config *config)
{
    config->size = sizeof(struct im_library);

    config->im.init = im_library_init;
    config->im.io = im_library_io;

    config->ui.alloc = ui_library_alloc;
    config->ui.free = ui_library_free;
    config->ui.update = ui_library_update;
    config->ui.event = ui_library_event;
    config->ui.render = ui_library_render;

    config->io.list = im_library_io_list;
    config->io.len = array_len(im_library_io_list);
}
