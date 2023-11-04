/* man.h
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct db_man_it
{
    const void *it, *end;
    size_t path_len; const char *path;
    size_t data_len; const char *data;
};

bool db_man_next(struct db_man_it *);
