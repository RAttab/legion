/* worker.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct obj;
struct hunk;


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct legion_packed worker
{
    id_t dock;
};

struct obj *worker_alloc(struct hunk *);
bool worker_io(
        struct obj *,
        struct hunk *,
        void *state,
        int64_t *buf,
        size_t len);
