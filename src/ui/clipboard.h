/* clipboard.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"

// -----------------------------------------------------------------------------
// clipboard
// -----------------------------------------------------------------------------

struct ui_clipboard
{
    size_t len, cap;
    char *str;
};

void ui_clipboard_init(struct ui_clipboard *);
void ui_clipboard_free(struct ui_clipboard *);

size_t ui_clipboard_paste(struct ui_clipboard *, size_t len, char *dst);

void ui_clipboard_copy(struct ui_clipboard *, size_t len, const char *src);
void ui_clipboard_copy_hex(struct ui_clipboard *, uint64_t val);
