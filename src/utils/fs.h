/* fs.h
   Rémi Attab (remi.attab@gmail.com), 31 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// dir_it
// -----------------------------------------------------------------------------

struct dir_it;

struct dir_it *dir_it(const char *path);
void dir_it_free(struct dir_it *);

bool dir_it_next(struct dir_it *);
const char *dir_it_path(struct dir_it *);


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

size_t file_len(int fd);

bool file_exists(const char *path);
void file_tmpbak_swap(const char *path);


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
