/* save_mem.c
   Rémi Attab (remi.attab@gmail.com), 20 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include <sys/mman.h>


// -----------------------------------------------------------------------------
// mem
// -----------------------------------------------------------------------------

struct save *save_mem_new(void)
{
    struct save *save = calloc(1, sizeof(*save));

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

    return save;

  fail_mmap:
    free(save);
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
    free(save);
}
