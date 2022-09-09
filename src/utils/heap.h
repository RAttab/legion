/* heap.h
   Rémi Attab (remi.attab@gmail.com), 16 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct save;


// -----------------------------------------------------------------------------
// heap
// -----------------------------------------------------------------------------

typedef uint32_t heap_ix;

struct heap
{
    heap_ix free[8];
    uint32_t len, cap;
    void *data;
};

void heap_init(struct heap *);
void heap_free(struct heap *);

void *heap_ptr(struct heap *, heap_ix);
heap_ix heap_index(struct heap *, const void *ptr);

heap_ix heap_new(struct heap *, size_t size);
void heap_del(struct heap *, heap_ix, size_t size);

void heap_save(struct heap *, struct save *);
bool heap_load(struct heap *, struct save *);
