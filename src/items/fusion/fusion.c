/* fusion.c
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


#include "items/fusion/fusion.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/fusion/fusion_im.c"
#include "items/fusion/fusion_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_fusion_config(struct im_config *config)
{
    config->size = sizeof(struct im_fusion);

    config->im.init = im_fusion_init;
    config->im.step = im_fusion_step;
    config->im.io = im_fusion_io;
    config->im.flow = im_fusion_flow;

    config->ui.alloc = ui_fusion_alloc;
    config->ui.free = ui_fusion_free;
    config->ui.update = ui_fusion_update;
    config->ui.render = ui_fusion_render;

    config->io.list = im_fusion_io_list;
    config->io.len = array_len(im_fusion_io_list);
}
