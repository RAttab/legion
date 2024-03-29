/* ui_str.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

struct ui_str ui_str_c(const char *str)
{
    size_t len = strnlen(str, ui_str_cap);
    assert(len <= ui_str_cap);
    return (struct ui_str) {
        .len = len,
        .cap = 0,
        .str = str,
    };
}

struct ui_str ui_str_v(size_t len)
{
    assert(len <= ui_str_cap);
    return (struct ui_str) {
        .len = 0,
        .cap = len,
        .str = mem_array_alloc_t(char, len),
    };
}

struct ui_str ui_str_clone(const struct ui_str *str)
{
    return str->cap ? ui_str_v(str->cap) : ui_str_c(str->str);
}

void ui_str_free(struct ui_str *str)
{
    if (str->cap) mem_free((char *) str->str);
}


void ui_str_copy(struct ui_str *str)
{
    ui_clipboard_copy(str->str, str->len);
}

void ui_str_paste(struct ui_str *str)
{
    assert(str->cap);
    str->len = ui_clipboard_paste((char *) str->str, str->cap);
}


// -----------------------------------------------------------------------------
// set
// -----------------------------------------------------------------------------

void ui_str_setc(struct ui_str *str, const char *val)
{
    str->len = legion_min(strlen(val), ui_str_cap);
    if (!str->cap) str->str = val;
    else memcpy((char *) str->str, val, str->len);
}

void ui_str_setv(struct ui_str *str, const char *val, size_t len)
{
    assert(len <= str->cap);
    str->len = len;
    memcpy((char *) str->str, val, len);
}

void ui_str_setvf(struct ui_str *str, const char *fmt, va_list args)
{
    assert(str->cap);
    ssize_t len = vsnprintf((char *)(str->str), str->cap, fmt, args);
    assert(len >= 0);
    str->len = len;
}

void ui_str_setf(struct ui_str *str, const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    ui_str_setvf(str, fmt, args);
    va_end(args);
}

void ui_str_set_nil(struct ui_str *str)
{
    ui_str_setc(str, "nil");
}

void ui_str_set_u64(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_utoa(val, (char *)str->str, legion_min(str->cap, 20));
}

void ui_str_set_hex(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_utox(val, (char *)str->str, legion_min(str->cap, 16));
}

void ui_str_set_scaled(struct ui_str *str, uint64_t val)
{
    assert(str->cap);
    str->len = str_scaled(val, (char *)str->str,  str->cap);
}

void ui_str_set_id(struct ui_str *str, im_id val)
{
    assert(str->cap);
    str->len = im_id_str(val, (char *) str->str, str->cap);
}

void ui_str_set_item(struct ui_str *str, enum item val)
{
    assert(str->cap);
    str->len = item_str(val, (char *) str->str, str->cap);
}

void ui_str_set_coord(struct ui_str *str, struct coord val)
{
    assert(str->cap);
    if (coord_is_nil(val)) ui_str_setc(str, "nil");
    else str->len = coord_str(val, (char *) str->str, str->cap);
}

void ui_str_set_coord_name(struct ui_str *str, struct coord val)
{
    assert(str->cap);
    if (coord_is_nil(val)) ui_str_setc(str, "nil");
    else ui_str_set_atom(str, proxy_star_name(val));
}

void ui_str_set_symbol(struct ui_str *str, const struct symbol *val)
{
    assert(str->cap);
    str->len = val->len;
    memcpy((char *) str->str, val->c, str->len);
}

void ui_str_set_atom(struct ui_str *str, vm_word word)
{
    struct symbol sym = {0};
    if (atoms_str(proxy_atoms(), word, &sym))
        ui_str_set_symbol(str, &sym);
    else ui_str_set_hex(str, word);
}
