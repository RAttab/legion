/* db.h
   Rémi Attab (remi.attab@gmail.com), 05 Apr 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"

// -----------------------------------------------------------------------------
// items
// -----------------------------------------------------------------------------

void db_items_load();
item_t db_items_from_schema(const schema_t schema);
