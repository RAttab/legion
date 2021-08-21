/* research.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/research/research.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/research/research_im.c"
#include "items/research/research_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_research_config(struct im_config *config)
{
    config->size = sizeof(struct im_research);

    config->im.init = im_research_init;
    config->im.step = im_research_step;
    config->im.io = im_research_io;
    config->im.flow = im_research_flow;

    config->ui.alloc = ui_research_alloc;
    config->ui.free = ui_research_free;
    config->ui.update = ui_research_update;
    config->ui.render = ui_research_render;

    config->io_list = im_research_io_list;
    config->io_list_len = array_len(im_research_io_list);
}
