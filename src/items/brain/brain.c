/* brain.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "items/brain/brain.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    im_brain_stack_s = 1,
    im_brain_stack_m = 2,
    im_brain_stack_l = 4,
    im_brain_stack_max = im_brain_stack_l,

    im_brain_speed_s = 2,
    im_brain_speed_m = 4,
    im_brain_speed_l = 6,

    im_brain_len_s = sizeof(struct im_brain) + vm_len(im_brain_stack_s),
    im_brain_len_m = sizeof(struct im_brain) + vm_len(im_brain_stack_m),
    im_brain_len_l = sizeof(struct im_brain) + vm_len(im_brain_stack_l),

};


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/brain/brain_im.c"
#include "items/brain/brain_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_brain_config(struct im_config *config)
{
    switch(config->type) {
    case ITEM_BRAIN_1: { config->size = im_brain_len_s; break; }
    case ITEM_BRAIN_2: { config->size = im_brain_len_m; break; }
    case ITEM_BRAIN_3: { config->size = im_brain_len_l; break; }
    default: { assert(false); }
    }

    config->gm.init = im_brain_init;
    config->gm.make = im_brain_make;
    config->gm.step = im_brain_step;
    config->gm.load = im_brain_load;
    config->gm.io = im_brain_io;

    config->ui.alloc = ui_brain_alloc;
    config->ui.free = ui_brain_free;
    config->ui.update = ui_brain_update;
    config->ui.event = ui_brain_event;
    config->ui.render = ui_brain_render;

    config->io_list = im_brain_io_list;
    config->io_list_len = array_len(im_brain_io_list);
}
