/* hunk.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "../utils.h"

#include "objects.h"

// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------


enum otype
{

};


// -----------------------------------------------------------------------------
// hunk
// -----------------------------------------------------------------------------

struct hunk
{
};


id_t hunk_id(struct hunk *, enum obj_type);
struct obj *hunk_obj(struct hunk *, id_t id);
struct obj *hunk_broadcast(struct hunk *, size_t len, int64_t msg);
