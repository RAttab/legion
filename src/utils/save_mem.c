/* save_mem.c
   Rémi Attab (remi.attab@gmail.com), 20 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include <sys/mman.h>


// -----------------------------------------------------------------------------
// mem
// -----------------------------------------------------------------------------

static void save_mem_grow(struct save *save, size_t len)
{
    size_t cap = save_cap(save);
    const size_t old = save_len(save);
    const size_t need = save_len(save) + len;

    assert(need > cap);
    while (need >= cap) cap += save_chunks;

    save->base = mremap(save->base, save_cap(save), cap, MREMAP_MAYMOVE);
    if (save->base == MAP_FAILED)
        failf_errno("unable to grow mem '%lx'", cap);

    save->it = save->base + old;
    save->end = save->base + cap;
}

struct save *save_mem_new(void)
{
    struct save *save = mem_alloc_t(save);

    const size_t cap = save_chunks;
    const int prot = PROT_READ | PROT_WRITE;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    save->base = mmap(0, cap, prot, flags, 0, 0);
    if (save->base == MAP_FAILED) {
        err_errno("unable to mmap 'mem'");
        goto fail_mmap;
    }

    save->end = save->base + cap;
    save->it = save->base;
    save->grow = save_mem_grow;

    return save;

  fail_mmap:
    mem_free(save);
    return NULL;
}

void save_mem_reset(struct save *save)
{
    save->it = save->base;
}

void save_mem_free(struct save *save)
{
    if (!save) return;
    munmap(save->base, save_cap(save));
    save_free(save);
    mem_free(save);
}
