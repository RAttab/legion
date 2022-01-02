/* fs.c
   Rémi Attab (remi.attab@gmail.com), 31 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/fs.h"
#include "utils/err.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>


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
    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) failf_errno("failed to stat: %d", fd);
    return stat.st_size;
}

bool file_tmpbak_swap(const char *path)
{
    char dst[PATH_MAX] = {0};
    strcpy(dst, basename(path));

    char src[PATH_MAX] = {0};
    snprintf(src, sizeof(src), "%s.tmp", dst);

    char bak[PATH_MAX] = {0};
    snprintf(bak, sizeof(bak), "%s.bak", dst);

    int fd = open(dirname(path), O_PATH);
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
