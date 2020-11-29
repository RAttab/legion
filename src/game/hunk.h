/* hunk.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "../utils.h"


// -----------------------------------------------------------------------------
// hunk
// -----------------------------------------------------------------------------

struct hunk;

struct hunk *hunk_alloc(struct coord);
void hunk_free(struct hunk *);

struct obj *hunk_obj_alloc(struct hunk *, size_t len);
struct obj *hunk_obj(struct hunk *, id_t id);

void hunk_step(struct hunk *);
