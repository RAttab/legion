/* utils.c
   Rémi Attab (remi.attab@gmail.com), 17 Sep 2017
   FreeBSD-style copyright and disclaimer apply
*/

#include "legion.h"
#include "utils.h"

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>


// -----------------------------------------------------------------------------
// error
// -----------------------------------------------------------------------------

__thread struct legion_error legion_errno = { 0 };

void legion_abort()
{
    legion_perror(&legion_errno);
    abort();
}

void legion_exit(int code)
{
    legion_perror(&legion_errno);
    exit(code);
}

size_t legion_strerror(struct legion_error *err, char *dest, size_t len)
{
    if (!err->errno_) {
        return snprintf(dest, len, "%s:%d: %s\n",
                err->file, err->line, err->msg);
    }
    else {
        return snprintf(dest, len, "%s:%d: %s - %s(%d)\n",
                err->file, err->line, err->msg,
                strerror(err->errno_), err->errno_);
    }
}

void legion_perror(struct legion_error *err)
{
    char buf[128 + legion_err_msg_cap];
    size_t len = legion_strerror(err, buf, sizeof(buf));

    if (write(2, buf, len) == -1)
        fprintf(stderr, "legion_perror failed: %s", strerror(errno));
}


void legion_vfail(const char *file, int line, const char *fmt, ...)
{
    legion_errno = (struct legion_error) { .errno_ = 0, .file = file, .line = line };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(legion_errno.msg, legion_err_msg_cap, fmt, args);
    va_end(args);
}

void legion_vfail_errno(const char *file, int line, const char *fmt, ...)
{
    legion_errno = (struct legion_error) { .errno_ = errno, .file = file, .line = line };

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(legion_errno.msg, legion_err_msg_cap, fmt, args);
    va_end(args);
}
