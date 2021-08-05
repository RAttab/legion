/* save.c
   Rémi Attab (remi.attab@gmail.com), 01 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/save.h"
#include "utils/vec.h"
#include "utils/ring.h"

#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
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
    uint8_t version;

    size_t cap;
    void *base, *it;

    int fd;
    char dst[PATH_MAX];
};

struct save *save_new(const char *path, uint8_t version)
{
    struct save *save = calloc(1, sizeof(*save));
    save->load = false;
    save->version = version;
    strcpy(save->dst, path);

    char tmp[PATH_MAX] = {0};
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    // need O_RD to mmap the fd because... reasons...
    save->fd = open(tmp, O_CREAT | O_TRUNC | O_RDWR, 0640);
    if (save->fd == -1) {
        err_errno("unable to open tmp file for '%s'", path);
        goto fail_open;
    }

    if (ftruncate(save->fd, save_chunks) == -1) {
        err_errno("unable to grow file '%lx'", save_chunks);
        goto fail_truncate;
    }

    save->cap = save_chunks;
    save->base = mmap(0, save->cap, PROT_WRITE, MAP_SHARED, save->fd, 0);
    if (save->base == MAP_FAILED) {
        err_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }

    save->it = save->base;

    save_write_value(save, save_magic_top);
    save_write_value(save, (typeof(save_magic_seal)) 0);
    save_write_value(save, save->version);

    return save;

    munmap(save->base, save->cap);
  fail_mmap:
  fail_truncate:
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
        err_errno("unable to open '%s'", path);
        goto fail_open;
    }

    struct stat stat = {0};
    int ret = fstat(save->fd, &stat);
    if (ret == -1) {
        err_errno("unable to stat '%s'", path);
        goto fail_stat;
    }
    save->cap = stat.st_size;

    save->base = mmap(0, save->cap, PROT_READ, MAP_PRIVATE, save->fd, 0);
    if (save->base == MAP_FAILED) {
        err_errno("unable to mmap '%s'", path);
        goto fail_mmap;
    }
    save->it = save->base;

    uint64_t magic = save_read_type(save, typeof(magic));
    if (magic != save_magic_top) {
        err_errno("invalid magic '%lx' on save '%s'", magic, path);
        goto fail_magic;
    }

    uint64_t seal = save_read_type(save, typeof(magic));
    if (seal != save_magic_seal) {
        err_errno("invalid seal '%lx' on save '%s'", seal, path);
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
    size_t len = save_len(save);
    if (ftruncate(save->fd, len) == -1)
        fail_errno("unable to truncate '%d' to '%zu'", save->fd, save->cap);

    if (msync(save->base, len, MS_SYNC) == -1)
        fail_errno("unable to msync '%d'", save->fd);

    save->it = save->base + sizeof(save_magic_top);
    save_write_value(save, save_magic_seal);

    if (msync(save->base, save_len(save), MS_SYNC) == -1)
        fail_errno("unable to msync header '%d'", save->fd);

    char dst[PATH_MAX] = {0};
    strcpy(dst, basename(save->dst));

    char src[PATH_MAX] = {0};
    snprintf(src, sizeof(src), "%s.tmp", dst);

    char bak[PATH_MAX] = {0};
    snprintf(bak, sizeof(bak), "%s.bak", dst);

    int fd = open(dirname(save->dst), O_PATH);
    if (fd == -1) fail_errno("unable open dir '%s' for file '%s'", save->dst, dst);

    (void) unlinkat(fd, bak, 0);
    (void) linkat(fd, dst, fd, bak, 0);
    (void) unlinkat(fd, dst, 0);

    if (linkat(fd, src, fd, dst, 0) == -1) {
        fail_errno("unable to link '%s/%s' to '%s/%s' (%d)",
                save->dst, src, save->dst, dst, fd);
    }

    if (unlinkat(fd, src, 0) == -1)
        err_errno("unable to unlink tmp file: '%s/%s' (%d)", save->dst, src, fd);

    close(fd);
}

void save_close(struct save *save)
{
    if (!save->load) save_seal(save);

    munmap(save->base, save->cap);
    close(save->fd);
    free(save);
}

bool save_eof(struct save *save)
{
    return save_len(save) == save->cap;
}

uint8_t save_version(struct save *save)
{
    return save->version;
}

size_t save_len(struct save *save)
{
    return save->it - save->base;
}

static void save_grow(struct save *save, size_t len)
{
    size_t need = save_len(save) + len;
    if (likely(need <= save->cap)) return;

    size_t it = save_len(save);
    size_t cap = save->cap;
    while (need >= cap) cap += save_chunks;

    if (ftruncate(save->fd, cap) == -1)
        fail_errno("unable to grow file '%lx'", cap);

    save->base = mremap(save->base, save->cap, cap, MREMAP_MAYMOVE);
    if (save->base == MAP_FAILED)
        fail_errno("unable to remap '%p' from '%zu' to '%zu'", save->base, save->cap, cap);

    save->cap = cap;
    save->it = save->base + it;
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
    assert(save_len(save) + len <= save->cap);

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

    err("invalid magic %x != %x", (unsigned) value, (unsigned) exp);
    return false;
}


void save_write_htable(struct save *save, const struct htable *ht)
{
    save_write_magic(save, save_magic_htable);
    save_write_value(save, (uint64_t) ht->len);

    for (struct htable_bucket *it = htable_next(ht, NULL);
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

void save_write_ring64(struct save *save, const struct ring64 *ring)
{
    save_write_magic(save, save_magic_ring64);
    save_write_value(save, ring->cap);
    save_write_value(save, ring->head);
    save_write_value(save, ring->tail);
    save_write(save, ring->vals, ring->cap * sizeof(ring->vals[0]));
    save_write_magic(save, save_magic_ring64);
}

struct ring64 *save_read_ring64(struct save *save)
{
    if (!save_read_magic(save, save_magic_ring64)) return NULL;

    struct ring64 *ring = ring64_reserve(save_read_type(save, typeof(ring->cap)));
    save_read_into(save, &ring->head);
    save_read_into(save, &ring->tail);
    save_read(save, ring->vals, ring->cap * sizeof(ring->vals[0]));

    if (!save_read_magic(save, save_magic_ring64)) { free(ring); return NULL; }
    return ring;
}

void save_write_symbol(struct save *save, const struct symbol *symbol)
{
    save_write_magic(save, save_magic_symbol);
    save_write_value(save, symbol->len);
    save_write(save, symbol->c, symbol->len);
    save_write_magic(save, save_magic_symbol);
}

bool save_read_symbol(struct save *save, struct symbol *dst)
{
    if (!save_read_magic(save, save_magic_symbol)) return false;
    save_read_into(save, &dst->len);
    save_read(save, dst->c, dst->len);
    return save_read_magic(save, save_magic_symbol);
}
