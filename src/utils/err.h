/* log.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include <errno.h>
#include <stdio.h>


// -----------------------------------------------------------------------------
// info
// -----------------------------------------------------------------------------

#define info(msg)                               \
    do {                                        \
        fprintf(stdout, "<inf> " msg "\n");     \
    } while(false)

#define infof(fmt, ...)                                         \
    do {                                                        \
        fprintf(stdout, "<inf> " fmt "\n", __VA_ARGS__);        \
    } while(false)


// -----------------------------------------------------------------------------
// err
// -----------------------------------------------------------------------------

#define err(msg)                                        \
    do {                                                \
        fprintf(stderr, "%s:%u: <err> " msg "\n",       \
                __FILE__, __LINE__);                    \
    } while(false)

#define errf(fmt, ...)                                  \
    do {                                                \
        fprintf(stderr, "%s:%u: <err> " fmt "\n",       \
                __FILE__, __LINE__, __VA_ARGS__);       \
    } while(false)

#define err_num(errnum, errstr, msg)                            \
    do {                                                        \
        fprintf(stderr, "%s:%u: <err> " msg ": %s (%d)\n",      \
                __FILE__, __LINE__, errstr, errnum);            \
    } while(false)

#define errf_num(errnum, errstr, fmt, ...)                              \
    do {                                                                \
        fprintf(stderr, "%s:%u: <err> " fmt ": %s (%d)\n",              \
                __FILE__, __LINE__, __VA_ARGS__, errstr, errnum);       \
    } while(false)


#define err_errno(msg) err_num(errno, strerror(errno), msg)
#define errf_errno(fmt, ...) \
    errf_num(errno, strerror(errno), fmt, __VA_ARGS__)

#define err_posix(errnum, msg) err_num(errnum, strerror(errnum), msg)
#define errf_posix(errnum, fmt, ...) \
    errf_num(errnum, strerror(errnum), fmt, __VA_ARGS__)


// -----------------------------------------------------------------------------
// fail
// -----------------------------------------------------------------------------

#define fail(msg)                               \
    do {                                        \
        err(msg);                               \
        abort();                                \
    } while(false)

#define failf(fmt, ...)                         \
    do {                                        \
        errf(fmt, __VA_ARGS__);                 \
        abort();                                \
    } while(false)

#define fail_errno(msg)                         \
    do {                                        \
        err_errno(msg);                         \
        abort();                                \
    } while(false)

#define failf_errno(fmt, ...)                   \
    do {                                        \
        errf_errno(fmt, __VA_ARGS__);           \
        abort();                                \
    } while(false)

#define fail_posix(errnum, msg)                 \
    do {                                        \
        err_posix(errnum, msg);                 \
        abort();                                \
    } while(false)

#define failf_posix(errnum, fmt, ...)           \
    do {                                        \
        errf_posix(errnum, fmt, __VA_ARGS__);   \
        abort();                                \
    } while(false)
