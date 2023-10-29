/* ui_clipboard.c
   Rémi Attab (remi.attab@gmail.com), 13 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// clipboard
// -----------------------------------------------------------------------------

struct
{
    size_t len, cap;
    char *str;
} ui_clipboard;


void ui_clipboard_init(void)
{
    ui_clipboard.len = 0;
    ui_clipboard.cap = 1024;
    ui_clipboard.str = calloc(ui_clipboard.cap, sizeof(*ui_clipboard.str));
}

void ui_clipboard_free(void)
{
    free(ui_clipboard.str);
}

size_t ui_clipboard_len(void)
{
    return ui_clipboard.len;
}

const char *ui_clipboard_str(void)
{
    return ui_clipboard.str;
}

size_t ui_clipboard_paste(char *dst, size_t len)
{
    len = legion_min(len, ui_clipboard.len);
    memcpy(dst, ui_clipboard.str, len);
    return len;
}

static void ui_clipboard_grow(size_t len)
{
    if (likely(len <= ui_clipboard.cap)) return;
    ui_clipboard.str = realloc(ui_clipboard.str, len);
    ui_clipboard.cap = len;
}

void ui_clipboard_copy(const char *src, size_t len)
{
    ui_clipboard_grow(len);
    memcpy(ui_clipboard.str, src, len);
    ui_clipboard.len = len;

    ui_log(st_info, "%zu characters copied", len);
}

void ui_clipboard_copy_hex(uint64_t val)
{
    ui_clipboard.len = u64_ceil_div(u64_log2(val), 4) + 2;
    ui_clipboard_grow(ui_clipboard.len);

    ui_clipboard.str[0] = '0';
    ui_clipboard.str[1] = 'x';
    str_utox(val, ui_clipboard.str + 2, ui_clipboard.len-2);
}

void ui_clipboard_clear(void)
{
    ui_clipboard.len = 0;
}
