/* extract.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/extract/extract.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/extract/extract_im.c"
#include "items/extract/extract_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_extract_config(struct im_config *config)
{
    config->size = sizeof(struct im_extract);

    config->im.init = im_extract_init;
    config->im.load = im_extract_load;
    config->im.step = im_extract_step;
    config->im.io = im_extract_io;
    config->im.flow = im_extract_flow;

    config->ui.alloc = ui_extract_alloc;
    config->ui.free = ui_extract_free;
    config->ui.update = ui_extract_update;
    config->ui.event = ui_extract_event;
    config->ui.render = ui_extract_render;

    config->io.list = im_extract_io_list;
    config->io.len = array_len(im_extract_io_list);
}
