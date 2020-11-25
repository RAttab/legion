/* worker.c
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "worker.h"

// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

struct obj *worker_alloc(id_t id)
{
    return obj_alloc(id, obj_worker, sizeof(struct worker), 0, 2, 0);
}

bool worker_io(struct obj *, struct hunk *, void *state, const int64_t *buf, size_t len)
{
    switch (buf[0]) {

    }
}
