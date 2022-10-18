/* fs.c
   Rémi Attab (remi.attab@gmail.com), 31 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/fs.h"
#include "utils/err.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>


// -----------------------------------------------------------------------------
// path
// -----------------------------------------------------------------------------


const char *path_home(void)
{
    const char *home = getenv("HOME");
    if (!home) fail("unable to determine home directory");
    return home;
}

bool path_is_dir(const char *path)
{
    struct stat ret = {0};
    if (stat(path, &ret) == -1)
        failf_errno("unable to stat '%s'", path);
    return S_ISDIR(ret.st_mode);
}

bool path_is_file(const char *path)
{
    struct stat ret = {0};
    if (stat(path, &ret) == -1)
        failf_errno("unable to stat '%s'", path);
    return S_ISREG(ret.st_mode);
}

// strncpy is nightmare fuel and I currently don't have strlcpy setup. So I'm
// using the slightly slower but safer combo strnlen + memcpy.
size_t path_concat(char *dst, size_t len, const char *base, const char *sub)
{
    size_t base_len = strnlen(base, len);
    size_t sub_len = strnlen(sub, len);

    size_t bytes = base_len + 1 + sub_len + 1;
    assert(bytes < len);

    memcpy(dst, base, base_len);
    *(dst + base_len) = '/';
    memcpy(dst + base_len + 1, sub, sub_len);
    *(dst + base_len + 1 + sub_len) = '\0';

    return bytes;
}


// -----------------------------------------------------------------------------
// dir_it
// -----------------------------------------------------------------------------

struct dir_it
{
    DIR *dir;

    char *it, *end;
    char file[PATH_MAX];
};

struct dir_it *dir_it(const char *path)
{
    struct dir_it *it = calloc(1, sizeof(*it));

    it->dir = opendir(path);
    if (!it->dir) failf_errno("can't open dir: %s", path);

    size_t len = strnlen(path, PATH_MAX);
    memcpy(it->file, path, len);

    it->it = it->file + len;
    it->end = it->file + sizeof(it->file);
    *(it->it) = '/'; it->it++;

    return it;
}

void dir_it_free(struct dir_it *it)
{
    closedir(it->dir);
    free(it);
}

bool dir_it_next(struct dir_it *it)
{
    struct dirent *entry = NULL;
    do { entry = readdir(it->dir); } while (entry && entry->d_name[0] == '.');
    if (!entry) return false;

    size_t len = strlen(entry->d_name); // the dirent struct enforces a 256 limit
    if (it->it + len >= it->end) {
        failf_errno("overly long path: %zu + %zu > %d",
                it->it - it->file, len, PATH_MAX);
    }

    memcpy(it->it, entry->d_name, len);
    it->it[len] = 0;

    return true;
}

const char *dir_it_path(struct dir_it *it)
{
    return it->file;
}


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

size_t file_len(int fd)
{
    struct stat val = {0};
    if (fstat(fd, &val) < 0) failf_errno("failed to stat fd '%d'", fd);
    return val.st_size;
}

bool file_exists(const char *path)
{
    struct stat val = {0};
    if (!stat(path, &val)) return true;
    if (errno == ENOENT) return false;

    failf_errno("unable to stat '%s'", path);
}

void file_truncate(const char *path, size_t len)
{
    if (truncate(path, len) < 0)
        failf_errno("unable to truncate the file '%s' to '%zu'", path, len);
}

int file_create_tmp(const char *path, size_t len)
{
    char tmp[PATH_MAX] = {0};
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    int fd = open(tmp, O_CREAT | O_TRUNC | O_RDWR, 0640);
    if (fd == -1) {
        errf_errno("unable to open tmp file for '%s'", path);
        goto fail_open;
    }

    if (ftruncate(fd, len) == -1) {
        errf_errno("unable to grow file '%lx'", len);
        goto fail_truncate;
    }

    return fd;

  fail_truncate:
    close(fd);
  fail_open:
    return -1;
}

void file_tmp_swap(const char *path)
{
    // basename and dirname are horrible functions. That's all I wanted or
    // needed to say.

    char basebuf[PATH_MAX] = {0};
    strcpy(basebuf, path);
    const char *base = basename(basebuf);

    char dirbuf[PATH_MAX] = {0};
    strcpy(dirbuf, path);
    const char *dir = dirname(dirbuf);

    char dst[PATH_MAX] = {0};
    strcpy(dst, base);

    char src[PATH_MAX] = {0};
    snprintf(src, sizeof(src), "%s.tmp", dst);

    char bak[PATH_MAX] = {0};
    snprintf(bak, sizeof(bak), "%s.bak", dst);

    int fd = open(dir, O_PATH);
    if (fd == -1) failf_errno("unable open dir '%s' for file '%s'", path, dst);

    (void) unlinkat(fd, bak, 0);
    if (!linkat(fd, dst, fd, bak, 0)) // success
        (void) unlinkat(fd, dst, 0);

    if (linkat(fd, src, fd, dst, 0) == -1) {
        failf_errno("unable to link '%s/%s' to '%s/%s' (%d)",
                path, src, path, dst, fd);
    }

    if (unlinkat(fd, src, 0) == -1)
        errf_errno("unable to unlink tmp file: '%s/%s' (%d)", path, src, fd);

    close(fd);
}


// -----------------------------------------------------------------------------
// mfile
// -----------------------------------------------------------------------------

struct mfile mfile_open(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) failf_errno("file not found: %s", path);

    struct mfile file = {0};

    file.len = file_len(fd);
    if (!file.len) failf("unable to mmap empty file: %s", path);

    file.ptr = mmap(0, file.len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (file.ptr == MAP_FAILED) failf_errno("failed to mmap: %s", path);

    close(fd);
    return file;
}

void mfile_close(struct mfile *file)
{
    munmap((void *) file->ptr, file->len);
}

// -----------------------------------------------------------------------------
// mfilew
// -----------------------------------------------------------------------------

struct mfilew mfilew_create_tmp(const char *path, size_t len)
{
    int fd = file_create_tmp(path, len);
    if (fd < 0) failf_errno("unable to create tmp file: %s", path);

    struct mfilew file = { .len = len };

    file.ptr = mmap(0, file.len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file.ptr == MAP_FAILED) failf_errno("failed to mmap: %s", path);

    close(fd);
    return file;
}

void mfilew_close(struct mfilew *file)
{
    munmap((void *) file->ptr, file->len);
}
