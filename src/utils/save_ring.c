/* save_ring.c
   Rémi Attab (remi.attab@gmail.com), 20 Jan 2022
   FreeBSD-style copyright and disclaimer apply

   Included in save.c
*/

#include <stdatomic.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/eventfd.h>


// -----------------------------------------------------------------------------
// ring
// -----------------------------------------------------------------------------
// Implemented via the classical Magic Ring Buffer which maps the same memfd
// area in two vmas back to back. Allows us to make a single read/write call to
// the kernel even when we're wrapping around the ring.

struct save_ring
{
    size_t cap;
    void *base, *loop;
    atomic_bool closed;
    struct { atomic_size_t pos; struct save save; } read, write;
    int wake;
};

// We rely on the fact that our cursors our 64 bits to avoid dealing with
// overflows.
static_assert(sizeof(atomic_size_t) == 8);

struct save_ring *save_ring_new(size_t cap)
{
    struct save_ring *ring = calloc(1, sizeof(*ring));

    // On linux, duplicating a VMA via mmap can only be achieved using an
    // fd. memfd_create allows us to get an fd without touching the file-system.
    int fd = memfd_create("legion.save.ring", MFD_CLOEXEC);
    if (fd == -1) {
        err_errno("unable to create memfd for ring");
        goto fail_memfd;
    }

    if (ftruncate(fd, cap) == -1) {
        err_errno("unable to resize memfd for ring");
        goto fail_ftruncate;
    }

    assert(cap && cap % s_page_len == 0);
    const int prot = PROT_READ | PROT_WRITE;

    // To ensure back-to-back VMAs without accidently overwritting any other
    // VMAs, we reserve a chunk of memory ahead of time.
    void *ptr = mmap(0, cap * 2, prot, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (ptr == MAP_FAILED) {
        err_errno("unable to reserve vma for ring");
        goto fail_mmap_reserve;
    }

    // MAP_FIXED is safe because we're within the bounds of our reserved
    // chunk. As a bonus, the two new VMAs completely replace the reserved VMA
    // so we don't need to clean it up.
    const int flags = MAP_SHARED_VALIDATE | MAP_FIXED;
    ring->base = mmap(ptr, cap, prot, flags, fd, 0);
    ring->loop = mmap(ptr + cap, cap, prot, flags, fd, 0);
    if (ring->base == MAP_FAILED || ring->loop == MAP_FAILED) {
        err_errno("unable to mmap file for ring");
        goto fail_mmap_ring;
    }

    ring->wake = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (ring->wake == -1) {
        err_errno("unable to create eventfd for ring");
        goto fail_eventfd;
    }

    // The VMAs carry a reference to the fd so we don't need to keep track of
    // the original reference.
    close(fd);

    ring->cap = cap;
    return ring;

    close(ring->wake);
  fail_eventfd:
  fail_mmap_ring:
    if (ring->base) munmap(ring->base, ring->cap);
    if (ring->loop) munmap(ring->loop, ring->cap);
  fail_mmap_reserve:
  fail_ftruncate:
    close(fd);
  fail_memfd:
    free(ring);
    return NULL;
}

void save_ring_free(struct save_ring *ring)
{
    close(ring->wake);
    munmap(ring->base, ring->cap);
    munmap(ring->loop, ring->cap);
    save_free(&ring->read.save);
    save_free(&ring->write.save);
    free(ring);
}

void save_ring_close(struct save_ring *ring)
{
    atomic_store_explicit(&ring->closed, true, memory_order_relaxed);
}

bool save_ring_closed(struct save_ring *ring)
{
    return atomic_load_explicit(&ring->closed, memory_order_relaxed);
}

struct save *save_ring_read(struct save_ring *ring)
{
    // Acquire is on the writes because that's the chunk of memory we could
    // potentially read without it being fully written. We don't care if the
    // previous chunk of read memory is fully read as we're not going to touch
    // it.
    uint64_t read = atomic_load_explicit(&ring->read.pos, memory_order_relaxed);
    uint64_t write = atomic_load_explicit(&ring->write.pos, memory_order_acquire);

    struct save *save = &ring->read.save;
    save->end = ring->base + (write % ring->cap);
    save->base = ring->base + (read % ring->cap);
    save->it = save->base;

    if (read != write && save->end <= save->it)
        save->end += ring->cap;

    assert(save->end >= save->it);
    assert((size_t) (save->end - save->base) <= ring->cap * 2);
    return save;
}

struct save *save_ring_write(struct save_ring *ring)
{
    // Acquire is on the writes because that's the chunk of memory we could
    // potentially overwrite as it's being read. We don't care if the previous
    // chunk of write memory is fully written as we're not going to touch it.
    uint64_t write = atomic_load_explicit(&ring->write.pos, memory_order_relaxed);
    uint64_t read = atomic_load_explicit(&ring->read.pos, memory_order_acquire);

    struct save *save = &ring->write.save;
    save->end = ring->base + (read % ring->cap);
    save->base = ring->base + (write % ring->cap);
    save->it = save->base;

    if (read == write || save->end < save->it)
        save->end += ring->cap;

    assert(save->end >= save->it);
    assert((size_t) (save->end - save->base) <= ring->cap * 2);
    return save;
}

void save_ring_commit(struct save_ring *ring, struct save *save)
{
    uint64_t delta = save->it - save->base;

    uint64_t read = 0, write = 0;
    if (save == &ring->write.save) {
        read = atomic_load_explicit(&ring->read.pos, memory_order_relaxed);
        write = atomic_fetch_add_explicit(&ring->write.pos, delta, memory_order_release);
        write += delta;
    }
    else if (save == &ring->read.save) {
        write = atomic_load_explicit(&ring->write.pos, memory_order_relaxed);
        read = atomic_fetch_add_explicit(&ring->read.pos, delta, memory_order_release);
        read += delta;
    }
    else assert(false);

    assert(read <= write);
}

size_t save_ring_consume(struct save *save, size_t len)
{
    if (save->it + len > save->end)
        len = save->end - save->it;

    save->it += len;
    return len;
}

int save_ring_wake_fd(struct save_ring *ring)
{
    return ring->wake;
}

void save_ring_wake_signal(struct save_ring *ring)
{
    uint64_t value = 1;
    ssize_t ret = write(ring->wake, &value, sizeof(value));
    if (ret == -1) fail_errno("unable to write to wake fd");
}

void save_ring_wake_drain(struct save_ring *ring)
{
    uint64_t value = 0;
    ssize_t ret = read(ring->wake, &value, sizeof(value));
    if (ret == -1 && errno != EAGAIN)
        fail_errno("unable to read from wake fd");
}
