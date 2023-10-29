/* brain.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "items/brain/brain.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

#define im_brain_len(stack) \
    (sizeof(struct im_brain) + sizeof(vm_word) * vm_stack_len(stack))

constexpr size_t im_brain_stack_base = 1;
constexpr size_t im_brain_stack_volume = 3;
constexpr size_t im_brain_stack_dense = 1;
constexpr size_t im_brain_stack_max = im_brain_stack_volume;

constexpr size_t im_brain_speed_base = 2;
constexpr size_t im_brain_speed_volume = 2;
constexpr size_t im_brain_speed_dense = 4;

constexpr size_t im_brain_len_base = im_brain_len(im_brain_stack_base);
constexpr size_t im_brain_len_volume = im_brain_len(im_brain_stack_volume);
constexpr size_t im_brain_len_dense = im_brain_len(im_brain_stack_dense);


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
    case item_brain: { config->size = im_brain_len_base; break; }
    default: { assert(false); }
    }

    config->im.init = im_brain_init;
    config->im.make = im_brain_make;
    config->im.step = im_brain_step;
    config->im.load = im_brain_load;
    config->im.io = im_brain_io;

    config->ui.alloc = ui_brain_alloc;
    config->ui.free = ui_brain_free;
    config->ui.update = ui_brain_update;
    config->ui.event = ui_brain_event;
    config->ui.render = ui_brain_render;

    config->io.list = im_brain_io_list;
    config->io.len = array_len(im_brain_io_list);
}
