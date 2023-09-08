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

size_t ui_clipboard_len(void);
const char *ui_clipboard_str(void);

size_t ui_clipboard_paste(char *dst, size_t len);
void ui_clipboard_copy(const char *src, size_t len);
void ui_clipboard_copy_hex(uint64_t val);

void ui_clipboard_clear(void);
