/* hunk.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/coord.h"

struct vec64;

// -----------------------------------------------------------------------------
// hunk
// -----------------------------------------------------------------------------

struct hunk;

struct hunk *hunk_alloc(const struct star *);
void hunk_free(struct hunk *);

struct star *hunk_star(struct hunk *);

struct obj *hunk_obj_alloc(struct hunk *, item_t type, size_t len);
struct obj *hunk_obj(struct hunk *, id_t id);

void hunk_step(struct hunk *);
size_t hunk_harvest(struct hunk *, item_t type, size_t count);

struct vec64 *hunk_list(struct hunk *);
