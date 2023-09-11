/* str.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"
#include "clipboard.h"

#include "vm/vm.h"
#include "utils/symbol.h"
#include "game/id.h"
#include "game/coord.h"
#include "db/items.h"


// -----------------------------------------------------------------------------
// str
// -----------------------------------------------------------------------------

struct ui_str
{
    uint8_t len, cap;
    const char *str;
};

constexpr size_t ui_str_cap = 128;

struct ui_str ui_str_c(const char *);
struct ui_str ui_str_c(const char *);
struct ui_str ui_str_v(size_t len);
struct ui_str ui_str_clone(const struct ui_str *);
void ui_str_free(struct ui_str *);

void ui_str_copy(struct ui_str *);
void ui_str_paste(struct ui_str *);

void ui_str_setc(struct ui_str *, const char *str);
void ui_str_setv(struct ui_str *, const char *str, size_t len);
void ui_str_setvf(struct ui_str *, const char *fmt, va_list);
void ui_str_setf(struct ui_str *, const char *fmt, ...) legion_printf(2, 3);
void ui_str_set_nil(struct ui_str *);
void ui_str_set_u64(struct ui_str *, uint64_t val);
void ui_str_set_hex(struct ui_str *, uint64_t val);
void ui_str_set_scaled(struct ui_str *, uint64_t val);
void ui_str_set_id(struct ui_str *, im_id val);
void ui_str_set_item(struct ui_str *, enum item val);
void ui_str_set_coord(struct ui_str *, struct coord val);
void ui_str_set_coord_name(struct ui_str *, struct coord val);
void ui_str_set_symbol(struct ui_str *, const struct symbol *val);
void ui_str_set_atom(struct ui_str *, vm_word word);

inline size_t ui_str_len(struct ui_str *str)
{
    return str->cap ? str->cap : str->len;
}
