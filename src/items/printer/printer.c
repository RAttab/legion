/* printer.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/printer/printer_im.c"
#include "items/printer/printer_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_printer_config(struct im_config *config)
{
    config->size = sizeof(struct im_printer);

    config->im.init = im_printer_init;
    config->im.load = im_printer_load;
    config->im.step = im_printer_step;
    config->im.io = im_printer_io;
    config->im.flow = im_printer_flow;

    config->ui.alloc = ui_printer_alloc;
    config->ui.free = ui_printer_free;
    config->ui.update = ui_printer_update;
    config->ui.event = ui_printer_event;
    config->ui.render = ui_printer_render;

    config->io.list = im_printer_io_list;
    config->io.len = array_len(im_printer_io_list);
}
