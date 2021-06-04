/* config.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "config.h"

// -----------------------------------------------------------------------------
// item_config
// -----------------------------------------------------------------------------

const struct item_config *item_config(item_t item)
{
    switch (item) {
    case ITEM_WORKER: return worker_config();
    case ITEM_PRINTER: return printer_config();
    case ITEM_MINER: return miner_config();
    case ITEM_DEPLOYER: return deployer_config();
    default: return NULL;
    }
}

