/* fs.h
   Rémi Attab (remi.attab@gmail.com), 31 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// path
// -----------------------------------------------------------------------------

bool path_is_dir(const char *path);
bool path_is_file(const char *path);
size_t path_concat(char *dst, size_t len, const char *base, const char *sub);
const char *path_home(void);


// -----------------------------------------------------------------------------
// dir_it
// -----------------------------------------------------------------------------

struct dir_it;

struct dir_it *dir_it(const char *path);
void dir_it_free(struct dir_it *);

bool dir_it_next(struct dir_it *);
const char *dir_it_name(struct dir_it *);
const char *dir_it_path(struct dir_it *);


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

size_t file_len(int fd);
size_t file_len_p(const char *path);

bool file_exists(const char *path);
void file_truncate(const char *path, size_t len);

int file_create_tmp(const char *path, size_t len);
void file_tmp_swap(const char *path);


// -----------------------------------------------------------------------------
// mfile
// -----------------------------------------------------------------------------

struct mfile
{
    const char *ptr;
    size_t len;
};

struct mfile mfile_open(const char *path);
void mfile_close(struct mfile *);


// -----------------------------------------------------------------------------
// mfilew
// -----------------------------------------------------------------------------

struct mfilew
{
    char *ptr;
    size_t len;
};

struct mfilew mfilew_create_tmp(const char *path, size_t len);
void mfilew_close(struct mfilew *);


// -----------------------------------------------------------------------------
// mfile_writer
// -----------------------------------------------------------------------------

struct mfile_writer
{
    struct mfilew mfile;
    char *it, *end;
    char path[PATH_MAX];
};

void mfile_writer_open(struct mfile_writer *, const char *path, size_t cap);
void mfile_writer_close(struct mfile_writer *);
struct mfile_writer *mfile_writer_reset(struct mfile_writer *);

#define mfile_write(_f, _str)                           \
    do {                                                \
        struct mfile_writer *f = (_f);                  \
        f->it += snprintf(f->it, f->end - f->it, _str); \
        assert(f->it < f->end);                         \
    } while (false)

#define mfile_writef(_f, _fmt, ...)                                     \
    do {                                                                \
        struct mfile_writer *f = (_f);                                  \
        f->it += snprintf(f->it, f->end - f->it, _fmt, __VA_ARGS__);    \
        assert(f->it < f->end);                                         \
    } while (false)
