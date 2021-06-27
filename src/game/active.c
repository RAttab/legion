/* active.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "active.h"

// -----------------------------------------------------------------------------
// item_config
// -----------------------------------------------------------------------------

const struct item_config *item_config(item_t item)
{
    const struct item_config *progable_config(item_t);
    const struct item_config *worker_config(void);
    const struct item_config *brain_config(item_t);
    const struct item_config *db_config(item_t);

    switch (item) {
    case ITEM_WORKER: return worker_config();

    case ITEM_PRINTER:
    case ITEM_MINER:
    case ITEM_DEPLOYER: return progable_config(item);

    case ITEM_BRAIN_S:
    case ITEM_BRAIN_M:
    case ITEM_BRAIN_L: return brain_config(item);

    case ITEM_DB_S:
    case ITEM_DB_M:
    case ITEM_DB_L: return db_config(item);

    default: return NULL;
    }
}

bool item_is_progable(item_t item)
{
    switch (item)
    {
    case ITEM_PRINTER:
    case ITEM_MINER:
    case ITEM_DEPLOYER: return true;

    default: return false;
    }
}

bool item_is_brain(item_t item)
{
    switch (item)
    {
    case ITEM_BRAIN_S:
    case ITEM_BRAIN_M:
    case ITEM_BRAIN_L: return true;

    default: return false;
    }
}

bool item_is_db(item_t item)
{
    switch (item)
    {
    case ITEM_DB_S:
    case ITEM_DB_M:
    case ITEM_DB_L: return true;

    default: return false;
    }
}
