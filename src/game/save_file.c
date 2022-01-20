/* save_file.c
   Rémi Attab (remi.attab@gmail.com), 20 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/fs.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>


// -----------------------------------------------------------------------------
// save_file
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
