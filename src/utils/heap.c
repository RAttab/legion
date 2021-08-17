/* heap.c
   Rémi Attab (remi.attab@gmail.com), 17 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/heap.h"
#include "utils/bits.h"
#include "game/save.h"

#include <sys/mman.h>

// -----------------------------------------------------------------------------
// heap
// -----------------------------------------------------------------------------

static const uint64_t heap_nil = UINT32_MAX;

void heap_init(struct heap *heap)
{
    *heap = (struct heap) {0};

    for (size_t i = 0; i < array_len(heap->free); i++)
        heap->free[i] = heap_nil;
}

void heap_free(struct heap *heap)
{
    munmap(heap->data, heap->cap);
}

void *heap_ptr(struct heap *heap, heap_index_t index)
{
    assert(index < heap->len);
    return heap->data + index;
}

heap_index_t heap_index(struct heap *heap, const void *ptr)
{
    assert(ptr >= heap->data && ptr < heap->data + heap->len);
    return ptr - heap->data;
}

static size_t heap_class(struct heap *heap, size_t size)
{
    size_t class = u64_ceil_div(size, 8);
    assert(class < array_len(heap->free));
    return class;
}

static void heap_grow(struct heap *heap, size_t class)
{
    if (likely(heap->free[class] != heap_nil)) return;

    if (heap->len == heap->cap) {
        size_t old_cap = heap->cap;
        heap->cap += s_page_len * 8;

        if (old_cap)
            heap->data = mremap(heap->data, old_cap, heap->cap, MREMAP_MAYMOVE);
        else {
            int prot = PROT_READ | PROT_WRITE;
            int flags = MAP_PRIVATE | MAP_ANONYMOUS;
            heap->data = mmap(0, heap->cap, prot, flags, -1, 0);
        }

        if (heap->data == MAP_FAILED) {
            fail_errno("unable to grow the heap '%p' to '%u'",
                    heap->data, heap->cap);
        }
    }

    heap_index_t *it = heap->data + heap->len;
    heap->len += s_page_len;
    const heap_index_t *end = heap->data + heap->len;

    heap->free[class] = heap_index(heap, it);
    for (; it < end; it++) {
        const heap_index_t *next = it + 1;
        *it = next < end ? heap_index(heap, next) : heap_nil;
    }
}

heap_index_t heap_new(struct heap *heap, size_t size)
{
    const size_t class = heap_class(heap, size);

    heap_grow(heap, class);
    heap_index_t index = heap->free[class];

    void *ptr = heap_ptr(heap, index);
    heap->free[class] = *((heap_index_t *) ptr);
    memset(ptr, 0, size);

    return index;
}

void heap_del(struct heap *heap, heap_index_t index, size_t size)
{
    assert(index % 8 == 0);
    assert(index < heap->len);
    const size_t class = heap_class(heap, size);

    void *ptr = heap_ptr(heap, index);
    *((heap_index_t *) ptr) = heap->free[class];
    heap->free[class] = index;
}


void heap_save(struct heap *heap, struct save *save)
{
    save_write_magic(save, save_magic_heap);
    save_write_value(save, heap->len);
    save_write(save, heap->data, heap->len);
    save_write(save, heap->free, sizeof(heap->free));
    save_write_magic(save, save_magic_heap);
}

bool heap_load(struct heap *heap, struct save *save)
{
    if (!save_read_magic(save, save_magic_heap)) return false;

    save_read_into(save, &heap->len);
    heap->cap = heap->len;

    if (heap->cap) {
        int prot = PROT_READ | PROT_WRITE;
        int flags = MAP_PRIVATE | MAP_ANONYMOUS;
        heap->data = mmap(0, heap->cap, prot, flags, -1, 0);
        if (heap->data == MAP_FAILED)
            fail_errno("unable to mmap heap '%u'", heap->cap);
    }

    save_read(save, heap->data, heap->cap);
    save_read(save, heap->free, sizeof(heap->free));

    return save_read_magic(save, save_magic_heap);
}
