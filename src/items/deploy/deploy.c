/* deploy.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/deploy/deploy_im.c"
#include "items/deploy/deploy_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_deploy_config(struct im_config *config)
{
    config->size = sizeof(struct im_deploy);

    config->im.init = im_deploy_init;
    config->im.step = im_deploy_step;
    config->im.io = im_deploy_io;
    config->im.flow = im_deploy_flow;

    config->ui.alloc = ui_deploy_alloc;
    config->ui.free = ui_deploy_free;
    config->ui.update = ui_deploy_update;
    config->ui.render = ui_deploy_render;

    config->io.list = im_deploy_io_list;
    config->io.len = array_len(im_deploy_io_list);
}
