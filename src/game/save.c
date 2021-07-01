/* save.c
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/save.h"
#include "utils/vec.h"
#include "utils/ring.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

static const uint64_t save_magic_top = 0xFF4E4F4947454CFF;
static const uint64_t save_magic_seal = 0xFF4C4547494F4EFF;
static const size_t save_chunks = 10 * page_len;

struct save
{
    bool load;
    int fd, fd_dst;
    size_t len, cap;
    void *base, *it;
    uint8_t version;
};


struct save *save_new(const char *path, uint8_t version)
{
    struct save *save = calloc(1, sizeof(*save));
    save->load = false;
    save->version = version;

    save->fd = open("legion.tmp", O_CREAT | O_EXCL | O_WRONLY | O_TMPFILE, 0640);
    if (save->fd == -1) {
        fail_errno("unable to open tmp file for '%s'", path);
        goto fail_open;
    }

    save->fd_dst = open(path, O_PATH | O_CREAT);
    if (save->fd_dst == -1) {
        fail_errno("unable to open '%s'", path);
        goto fail_open_dst;
    }

    save->cap = save_chunks;
    save->base = mmap(0, save->cap, PROT_WRITE, MAP_SHARED, save->fd, 0);
    if (save->base == MAP_FAILED) {
        fail_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }

    save->len = 0;
    save->it = save->base;

    save_write_value(save, save_magic_top);
    save_write_value(save, (typeof(save_magic_seal)) 0);
    save_write_value(save, save->version);

    return save;

    munmap(save->base, save->cap);
  fail_mmap:
    close(save->fd_dst);
  fail_open_dst:
    close(save->fd);
  fail_open:
    free(save);
    return NULL;
}

struct save *save_load(const char *path)
{
    struct save *save = calloc(1, sizeof(*save));
    save->load = true;

    save->fd = open(path, O_RDONLY);
    if (save->fd == -1) {
        fail_errno("unable to open '%s'", path);
        goto fail_open;
    }

    struct stat stat = {0};
    int ret = fstat(save->fd, &stat);
    if (ret == -1) {
        fail_errno("unable to stat '%s'", path);
        goto fail_stat;
    }
    save->cap = stat.st_size;

    save->base = mmap(0, save->len, PROT_READ, MAP_PRIVATE, save->fd, 0);
    if (save->base == MAP_FAILED) {
        fail_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }

    save->len = save->cap;
    save->it = save->base;

    uint64_t magic = save_read_type(save, typeof(magic));
    if (magic != save_magic_top) {
        fail_errno("invalid magic '%lx' on save '%s'", magic, path);
        goto fail_magic;
    }

    uint64_t seal = save_read_type(save, typeof(magic));
    if (seal != save_magic_seal) {
        fail_errno("invalid seal '%lx' on save '%s'", seal, path);
        goto fail_seal;
    }

    save_read_into(save, &save->version);

    return save;

  fail_seal:
  fail_magic:
    munmap(save->base, save->cap);
  fail_mmap:
  fail_stat:
    close(save->fd);
  fail_open:
    free(save);
    return NULL;
}

static void save_seal(struct save *save)
{
    if (ftruncate(save->fd, save->len) == -1) {
        fail_errno("unable to truncate '%d' to '%zu'", save->fd, save->len);
        assert(false);
    }

    if (msync(save->base, save->len, MS_SYNC) == -1) {
        fail_errno("unable to msync '%d'", save->fd);
        assert(false);
    }

    save->it = save->base + sizeof(save_magic_top);
    save_write_value(save, save_magic_seal);

    if (msync(save->base, page_len, MS_SYNC) == -1) {
        fail_errno("unable to msync header '%d'", save->fd);
        assert(false);
    }

    if (linkat(save->fd, "", save->fd_dst, "", AT_EMPTY_PATH) == -1) {
        fail_errno("unable to link '%d' to '%d'", save->fd, save->fd_dst);
        assert(false);
    }
}

void save_close(struct save *save)
{
    if (!save->load) save_seal(save);

    munmap(save->base, save->cap);
    close(save->fd_dst);
    close(save->fd);
    free(save);
}

bool save_eof(struct save *save)
{
    return save->len == save->cap;
}

uint8_t save_version(struct save *save)
{
    return save->version;
}


static void save_grow(struct save *save, size_t len)
{
    if (likely(save->len + len <= save->cap)) return;

    size_t cap = save->cap;
    while (save->len + len >= cap) cap += save_chunks;

    save->base = mremap(save->base, save->cap, cap, MREMAP_MAYMOVE);
    if (save->base == MAP_FAILED) {
        fail_errno("unable to remap '%p' from '%zu' to '%zu'",
                save->base, save->cap, cap);
        assert(false);
    }

    save->cap = cap;
    save->it = save->base + save->len;
}


void save_write(struct save *save, const void *src, size_t len)
{
    assert(!save->load);
    save_grow(save, len);

    memcpy(save->it, src, len);
    save->it += len;
}

void save_read(struct save *save, void *dst, size_t len)
{
    assert(save->load);
    memcpy(dst, save->it, len);
    save->it += len;
}


void save_write_magic(struct save *save, enum save_magic value)
{
    save_write(save, &value, sizeof(value));
}

bool save_read_magic(struct save *save, enum save_magic exp)
{
    enum save_magic value = 0;
    save_read(save, &value, sizeof(value));
    if (likely(value == exp)) return true;

    dbg("invalid magic %x != %x", (unsigned) value, (unsigned) exp);
    return false;
}



void save_write_vec64(struct save *save, const struct vec64 *vec)
{
    save_write_magic(save, save_magic_vec64);
    save_write_value(save, vec->len);
    save_write(save, vec->vals, vec->len * sizeof(vec->vals[0]));
    save_write_magic(save, save_magic_vec64);
}

struct vec64 *save_read_vec64(struct save *save)
{
    if (!save_read_magic(save, save_magic_vec64)) return NULL;

    struct vec64 *vec = vec64_reserve(save_read_type(save, typeof(vec->len)));
    save_read(save, vec->vals, vec->len * sizeof(vec->vals[0]));

    if (!save_read_magic(save, save_magic_vec64)) { free(vec); return NULL; }
    return vec;
}


void save_write_ring32(struct save *save, const struct ring32 *ring)
{
    save_write_magic(save, save_magic_ring32);
    save_write_value(save, ring->cap);
    save_write_value(save, ring->head);
    save_write_value(save, ring->tail);
    save_write(save, ring->vals, ring->cap * sizeof(ring->vals[0]));
    save_write_magic(save, save_magic_ring32);
}

struct ring32 *save_read_ring32(struct save *save)
{
    if (!save_read_magic(save, save_magic_ring32)) return NULL;

    struct ring32 *ring = ring32_reserve(save_read_type(save, typeof(ring->cap)));
    save_read_into(save, &ring->head);
    save_read_into(save, &ring->tail);
    save_read(save, ring->vals, ring->cap * sizeof(ring->vals[0]));

    if (!save_read_magic(save, save_magic_ring32)) { free(ring); return NULL; }
    return ring;
}
