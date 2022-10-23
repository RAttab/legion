/* clipboard.c
   Rémi Attab (remi.attab@gmail.com), 13 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "clipboard.h"


// -----------------------------------------------------------------------------
// clipboard
// -----------------------------------------------------------------------------

void ui_clipboard_init(struct ui_clipboard *board)
{
    board->len = 0;
    board->cap = 128;
    board->str = calloc(board->cap, sizeof(*board->str));
}

void ui_clipboard_free(struct ui_clipboard *board)
{
    free(board->str);
}

size_t ui_clipboard_paste(struct ui_clipboard *board, size_t len, char *dst)
{
    len = legion_min(len, board->len);
    memcpy(dst, board->str, len);
    return len;
}

static void ui_clipboard_grow(struct ui_clipboard *board, size_t len)
{
    if (likely(len <= board->cap)) return;
    board->str = realloc(board->str, len);
    board->cap = len;
}

void ui_clipboard_copy(struct ui_clipboard *board, size_t len, const char *src)
{
    ui_clipboard_grow(board, len);
    memcpy(board->str, src, len);
    board->len = len;
}

void ui_clipboard_copy_hex(struct ui_clipboard *board, uint64_t val)
{
    board->len = u64_ceil_div(u64_log2(val), 4) + 2;
    ui_clipboard_grow(board, board->len);

    board->str[0] = '0';
    board->str[1] = 'x';
    str_utox(val, board->str + 2, board->len-2);
}
