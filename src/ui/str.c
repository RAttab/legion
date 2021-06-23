/* str.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "utils/str.h"
#include "game/item.h"

#include <stdarg.h>


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

struct ui_str ui_str_c(const char *str)
{
    size_t len = strnlen(str, ui_str_cap);
    return (struct ui_str) {
        .len = len,
        .cap = 0,
        .str = str,
    };
}

struct ui_str ui_str_v(size_t len)
{
    return (struct ui_str) {
        .len = 0,
        .cap = len,
        .str = calloc(len, sizeof(char)),
    };
}

struct ui_str ui_str_clone(const struct ui_str *str)
{
    return str->cap ? ui_str_v(str->cap) : ui_str_c(str->str);
}

void ui_str_free(struct ui_str *str)
{
    if (str->cap) free((char *) str->str);
}

void ui_str_setc(struct ui_str *str, const char *val)
{
    str->len = strnlen(val, ui_str_cap);
    if (!str->cap) str->str = val;
    else memcpy((char *) str->str, val, str->len);
}

void ui_str_setv(struct ui_str *str, const char *val, size_t len)
{
    assert(len <= str->cap);
    str->len = len;
    memcpy((char *) str->str, val, len);
}

void ui_str_setf(struct ui_str *str, const char *fmt, ...)
{
    assert(str->cap);

    va_list args;
    va_start(args, fmt);
    ssize_t len = vsnprintf((char *)(str->str), str->cap, fmt, args);
    va_end(args);

    assert(len >= 0);
    str->len = len;
}

void ui_str_set_u64(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_utoa(val, (char *)str->str, str->cap);
}

void ui_str_set_hex(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_utox(val, (char *)str->str,  str->cap);
}

void ui_str_set_scaled(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_scaled(val, (char *)str->str,  str->cap);
}

void ui_str_set_id(struct ui_str *str, id_t val)
{
    assert(str->cap);
    str->len = id_str(val, str->cap, (char *) str->str);
}
