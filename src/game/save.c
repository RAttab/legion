/* save.c
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/save.h"
#include "utils/vec.h"
#include "utils/err.h"

#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/eventfd.h>


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

static const uint64_t save_magic_top = 0xFF4E4F4947454CFF;
static const uint64_t save_magic_seal = 0xFF4C4547494F4EFF;
static const size_t save_chunks = 10 * page_len;

typedef void (*save_grow_fn_t) (struct save *, size_t len);

struct save_prof
{
    size_t depth;
    struct { enum save_magic magic; size_t it; } stack[16];
    size_t bytes[save_magic_len];
};

struct save
{
    void *base, *end, *it;
    save_grow_fn_t grow;
    struct save_prof *prof;
};

static void save_free(struct save *save)
{
    free(save->prof);
}

bool save_eof(struct save *save)
{
    return save_len(save) == save_cap(save);
}

size_t save_cap(struct save *save)
{
    return save->end - save->base;
}

size_t save_len(struct save *save)
{
    return save->it - save->base;
}

void *save_bytes(struct save *save)
{
    return save->base;
}


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


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

enum save_mode
{
    save_mode_nil = 0,
    save_mode_write,
    save_mode_read,
};

struct save_file
{
    int fd;
    enum save_mode mode;
    char dst[PATH_MAX];
    uint8_t version;
};

static struct save_file *save_file_ptr(struct save *save)
{
    return (struct save_file *) (save + 1);
}

static void save_file_grow(struct save *save, size_t len)
{
    struct save_file *file = save_file_ptr(save);
    assert(file->mode == save_mode_write);

    size_t cap = save_cap(save);
    const size_t old = save_len(save);
    const size_t need = save_len(save) + len;

    assert(need > cap);
    while (need >= cap) cap += save_chunks;

    if (file->fd && ftruncate(file->fd, cap) == -1)
        failf_errno("unable to grow file '%lx'", cap);

    save->base = mremap(save->base, save_cap(save), cap, MREMAP_MAYMOVE);
    if (save->base == MAP_FAILED) {
        failf_errno("unable to remap '%p' from '%zu' to '%zu'",
                save->base, save_cap(save), cap);
    }

    save->it = save->base + old;
    save->end = save->base + cap;
}

struct save *save_file_create(const char *path, uint8_t version)
{
    struct save *save = calloc(1, sizeof(*save) + sizeof(struct save_file));
    struct save_file *file = save_file_ptr(save);

    file->mode = save_mode_write;
    file->version = version;
    strcpy(file->dst, path);
    save->grow = save_file_grow;

    char tmp[PATH_MAX] = {0};
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    // need O_RD to mmap the fd because... reasons...
    file->fd = open(tmp, O_CREAT | O_TRUNC | O_RDWR, 0640);
    if (file->fd == -1) {
        errf_errno("unable to open tmp file for '%s'", path);
        goto fail_open;
    }

    if (ftruncate(file->fd, save_chunks) == -1) {
        errf_errno("unable to grow file '%lx'", save_chunks);
        goto fail_truncate;
    }

    const size_t cap = save_chunks;
    save->base = mmap(0, cap, PROT_WRITE, MAP_SHARED, file->fd, 0);
    if (save->base == MAP_FAILED) {
        errf_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }

    save->end = save->base + cap;
    save->it = save->base;

    save_write_value(save, save_magic_top);
    save_write_value(save, (typeof(save_magic_seal)) 0);
    save_write_value(save, file->version);

    return save;

    munmap(save->base, save_cap(save));
  fail_mmap:
  fail_truncate:
    close(file->fd);
  fail_open:
    save_free(save);
    return NULL;
}

struct save *save_file_load(const char *path)
{
    struct save *save = calloc(1, sizeof(*save) + sizeof(struct save_file));
    struct save_file *file = save_file_ptr(save);

    file->mode = save_mode_read;
    save->grow = save_file_grow;

    file->fd = open(path, O_RDONLY);
    if (file->fd == -1) {
        errf_errno("unable to open '%s'", path);
        goto fail_open;
    }

    struct stat stat = {0};
    int ret = fstat(file->fd, &stat);
    if (ret == -1) {
        errf_errno("unable to stat '%s'", path);
        goto fail_stat;
    }

    const size_t cap = stat.st_size;
    save->base = mmap(0, cap, PROT_READ, MAP_PRIVATE, file->fd, 0);
    if (save->base == MAP_FAILED) {
        errf_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }

    save->end = save->base + cap;
    save->it = save->base;

    uint64_t magic = save_read_type(save, typeof(magic));
    if (magic != save_magic_top) {
        errf_errno("invalid magic '%lx' on save '%s'", magic, path);
        goto fail_magic;
    }

    uint64_t seal = save_read_type(save, typeof(magic));
    if (seal != save_magic_seal) {
        errf_errno("invalid seal '%lx' on save '%s'", seal, path);
        goto fail_seal;
    }

    save_read_into(save, &file->version);

    return save;

  fail_seal:
  fail_magic:
    munmap(save->base, save_cap(save));
  fail_mmap:
  fail_stat:
    close(file->fd);
  fail_open:
    save_free(save);
    return NULL;
}

static void save_file_seal(struct save *save)
{
    struct save_file *file = save_file_ptr(save);
    if (file->mode != save_mode_write) return;

    size_t len = save_len(save);
    if (ftruncate(file->fd, len) == -1)
        failf_errno("unable to truncate '%d' to '%zu'", file->fd, len);

    if (msync(save->base, len, MS_SYNC) == -1)
        failf_errno("unable to msync '%d'", file->fd);

    save->it = save->base + sizeof(save_magic_top);
    save_write_value(save, save_magic_seal);

    if (msync(save->base, save_len(save), MS_SYNC) == -1)
        failf_errno("unable to msync header '%d'", file->fd);

    file_tmpbak_swap(file->dst);
}

void save_file_close(struct save *save)
{
    save_file_seal(save);
    munmap(save->base, save_cap(save));
    close(save_file_ptr(save)->fd);
    save_free(save);
    free(save);
}

uint8_t save_file_version(struct save *save)
{
    return save_file_ptr(save)->version;
}


// -----------------------------------------------------------------------------
// read/write
// -----------------------------------------------------------------------------

size_t save_write(struct save *save, const void *src, size_t len)
{
    if (unlikely(save->it + len > save->end)) {
        if (save->grow) save->grow(save, len);
        else len = save->end - save->it;
    }

    memcpy(save->it, src, len);
    save->it += len;
    return len;
}

size_t save_read(struct save *save, void *dst, size_t len)
{
    if (unlikely(save->it + len > save->end))
        len = save->end - save->it;

    assert(save_len(save) + len <= save_cap(save));
    memcpy(dst, save->it, len);
    save->it += len;
    return len;
}


void save_write_magic(struct save *save, enum save_magic value)
{
    save_write(save, &value, sizeof(value));

    if (unlikely(save->prof != NULL)) {
        struct save_prof *prof = save->prof;
        const size_t prev = prof->depth - 1;
        const size_t it = save_len(save);

        if (prof->depth && prof->stack[prev].magic == value) {
            prof->bytes[value] += it - prof->stack[prev].it;
            prof->depth--;
        }
        else {
            assert(prof->depth < array_len(prof->stack));
            prof->stack[prof->depth].magic = value;
            prof->stack[prof->depth].it = it - 1;
            prof->depth++;
        }
    }
}

bool save_read_magic(struct save *save, enum save_magic exp)
{
    enum save_magic value = 0;
    save_read(save, &value, sizeof(value));
    if (likely(value == exp)) return true;

    errf("invalid magic %x != %x", (unsigned) value, (unsigned) exp);
    return false;
}


void save_write_htable(struct save *save, const struct htable *ht)
{
    save_write_magic(save, save_magic_htable);
    save_write_value(save, (uint64_t) ht->len);

    for (const struct htable_bucket *it = htable_next(ht, NULL);
         it; it = htable_next(ht, it))
    {
        save_write_value(save, it->key);
        save_write_value(save, it->value);
    }

    save_write_magic(save, save_magic_htable);
}

bool save_read_htable(struct save *save, struct htable *ht)
{
    if (!save_read_magic(save, save_magic_htable)) return false;

    size_t len = save_read_type(save, uint64_t);
    htable_reset(ht);
    htable_reserve(ht, len);

    for (size_t i = 0; i < len; ++i) {
        struct htable_bucket bucket = {0};
        save_read_into(save, &bucket.key);
        save_read_into(save, &bucket.value);

        struct htable_ret ret = htable_put(ht, bucket.key, bucket.value);
        assert(ret.ok);
    }

    return save_read_magic(save, save_magic_htable);
}


void save_write_vec64(struct save *save, const struct vec64 *vec)
{
    save_write_magic(save, save_magic_vec64);

    if (!vec) {
        save_write_value(save, (typeof(vec->len)) 0);
        save_write_value(save, (typeof(vec->cap)) 0);
    }
    else {
        save_write_value(save, vec->len);
        save_write_value(save, vec->cap);
        save_write(save, vec->vals, vec->len * sizeof(vec->vals[0]));
    }

    save_write_magic(save, save_magic_vec64);
}

bool save_read_vec64(struct save *save, struct vec64 **ret)
{
    if (!save_read_magic(save, save_magic_vec64)) return false;

    struct vec64 *vec = *ret;
    typeof(vec->len) len = save_read_type(save, typeof(len));
    typeof(vec->cap) cap = save_read_type(save, typeof(cap));

    if (cap) vec = vec64_grow(vec, cap);
    else if (vec && !cap) { vec64_free(vec); vec = NULL; }

    if (vec) {
        vec->len = len;
        save_read(save, vec->vals, len * sizeof(vec->vals[0]));
    }

    *ret = vec;
    return save_read_magic(save, save_magic_vec64);
}


// -----------------------------------------------------------------------------
// prof
// -----------------------------------------------------------------------------

void save_prof(struct save *save)
{
    if (!save->prof) save->prof = calloc(1, sizeof(*save->prof));
    else {
        memset(save->prof->bytes, 0, sizeof(save->prof->bytes));
        save->prof->depth = 0;
    }
}

void save_prof_dump(struct save *save)
{
    if (!save->prof) return;
    struct save_prof *prof = save->prof;

    char buffer[s_page_len] = {0};
    char *it = buffer;
    const char *end = it + sizeof(buffer);

    it += snprintf(it, end - it, "save: {total:");
    it += str_scaled(save_len(save), it, end - it);

    for (enum save_magic magic = 0; magic < save_magic_len; ++magic) {
        if (!prof->bytes[magic]) continue;

        const char *str = "";
        switch(magic) {
        case save_magic_vec64:  { str = "vec"; break; }
        case save_magic_ring32: { str = "rg3"; break; }
        case save_magic_ring64: { str = "rg6"; break; }
        case save_magic_htable: { str = " ht"; break; }
        case save_magic_symbol: { str = "sym"; break; }
        case save_magic_heap:   { str = " hp"; break; }

        case save_magic_sim:      { str = "sim"; break; }
        case save_magic_world:    { str = "wrd"; break; }
        case save_magic_star:     { str = "str"; break; }
        case save_magic_lab:      { str = "lab"; break; }
        case save_magic_tech:     { str = "tch"; break; }
        case save_magic_log:      { str = "log"; break; }
        case save_magic_lanes:    { str = "lns"; break; }
        case save_magic_lane:     { str = " ln"; break; }
        case save_magic_tape_set: { str = "tps"; break; }

        case save_magic_atoms:  { str = "atm"; break; }
        case save_magic_mods:   { str = "mds"; break; }
        case save_magic_mod:    { str = "mod"; break; }
        case save_magic_chunks: { str = "cks"; break; }
        case save_magic_chunk:  { str = " ck"; break; }
        case save_magic_active: { str = "act"; break; }
        case save_magic_energy: { str = "nrg"; break; }

        case save_magic_state_world:   { str = "swd"; break; }
        case save_magic_state_compile: { str = "scp"; break; }
        case save_magic_state_mod:     { str = "smd"; break; }
        case save_magic_state_chunk:   { str = "sck"; break; }

        default: { assert(false); }
        }

        it += snprintf(it, end - it, "} {%s:", str);
        it += str_scaled(prof->bytes[magic], it, end - it);
    }

    it += snprintf(it, end - it, "}\n");
    fprintf(stderr, "%s", buffer);

    free(save->prof);
    save->prof = NULL;
}
