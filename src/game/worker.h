/* worker.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "obj.h"
#include "hunk.h"

// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct worker
{
    id_t dock;
};

struct obj *worker_alloc(id_t id);

bool worker_io(struct obj *, struct hunk *, void *state, const int64_t *buf, size_t len);
