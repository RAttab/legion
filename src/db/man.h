/* man.h
   Rémi Attab (remi.attab@gmail.com), 22 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct man_db_it
{
    const void *it, *end;
    size_t path_len; const char *path;
    size_t data_len; const char *data;
};

bool man_db_next(struct man_db_it *);
