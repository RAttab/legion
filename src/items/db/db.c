/* db.c
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/db/db.h"
#include "items/config.h"

// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    im_db_len_s = 7 + 8 * 0,
    im_db_len_m = 7 + 8 * 1,
    im_db_len_l = 7 + 8 * 2,

    im_db_len_max = im_db_len_l,
};

// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/db/db_im.c"
#include "items/db/db_ui.c"


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_db_config(struct im_config *config)
{
    config->size = sizeof(struct im_db);

    switch(config->type) {
    case ITEM_DB_1: { config->size += im_db_len_s * sizeof(word_t); break; }
    case ITEM_DB_2: { config->size += im_db_len_m * sizeof(word_t); break; }
    case ITEM_DB_3: { config->size += im_db_len_l * sizeof(word_t); break; }
    default: { assert(false); }
    }

    config->gm.init = im_db_init;
    config->gm.make = im_db_make;
    config->gm.io = im_db_io;

    config->ui.alloc = ui_db_alloc;
    config->ui.free = ui_db_free;
    config->ui.update = ui_db_update;
    config->ui.event = ui_db_event;
    config->ui.render = ui_db_render;

    config->io_list = im_db_io_list;
    config->io_list_len = array_len(im_db_io_list);
}
