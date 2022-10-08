/* prober.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/prober/prober.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/prober/prober_im.c"
#include "items/prober/prober_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_prober_config(struct im_config *config)
{
    config->size = sizeof(struct im_prober);

    config->im.init = im_prober_init;
    config->im.step = im_prober_step;
    config->im.io = im_prober_io;

    config->ui.alloc = ui_prober_alloc;
    config->ui.free = ui_prober_free;
    config->ui.update = ui_prober_update;
    config->ui.event = ui_prober_event;
    config->ui.render = ui_prober_render;

    config->io.list = im_prober_io_list;
    config->io.len = array_len(im_prober_io_list);
}
