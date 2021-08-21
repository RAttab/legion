/* scanner.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/scanner/scanner.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/scanner/scanner_im.c"
#include "items/scanner/scanner_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_scanner_config(struct im_config *config)
{
    config->size = sizeof(struct im_scanner);

    config->im.init = im_scanner_init;
    config->im.step = im_scanner_step;
    config->im.io = im_scanner_io;

    config->ui.alloc = ui_scanner_alloc;
    config->ui.free = ui_scanner_free;
    config->ui.update = ui_scanner_update;
    config->ui.event = ui_scanner_event;
    config->ui.render = ui_scanner_render;

    config->io_list = im_scanner_io_list;
    config->io_list_len = array_len(im_scanner_io_list);
}
