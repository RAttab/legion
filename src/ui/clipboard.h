/* clipboard.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"

// -----------------------------------------------------------------------------
// clipboard
// -----------------------------------------------------------------------------

void ui_clipboard_init(void);
void ui_clipboard_free(void);

size_t ui_clipboard_paste(size_t len, char *dst);

void ui_clipboard_copy(size_t len, const char *src);
void ui_clipboard_copy_hex(uint64_t val);
