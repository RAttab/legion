/* burner.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/burner/burner.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/burner/burner_im.c"
#include "items/burner/burner_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_burner_config(struct im_config *config)
{
    config->size = sizeof(struct im_burner);

    config->im.init = im_burner_init;
    config->im.step = im_burner_step;
    config->im.io = im_burner_io;
    config->im.flow = im_burner_flow;

    config->ui.alloc = ui_burner_alloc;
    config->ui.free = ui_burner_free;
    config->ui.update = ui_burner_update;
    config->ui.render = ui_burner_render;

    config->io.list = im_burner_io_list;
    config->io.len = array_len(im_burner_io_list);
}
