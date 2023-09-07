/* mod.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "utils/symbol.h"
#include "utils/hash.h"
#include "game/user.h"


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

struct mods;
struct save;
struct text;
struct atoms;

typedef uint16_t mod_maj;
typedef uint16_t mod_ver;

inline mod_id make_mod(mod_maj maj, mod_ver ver)
{
    assert(!(maj >> 15));
    return maj << 16 | ver;
}

inline mod_maj mod_major(mod_id mod) { return mod >> 16; }
inline mod_ver mod_version(mod_id mod) { return ((1 << 16) - 1) & mod; }
inline bool mod_validate(vm_word word) { return word > 0 && word <= UINT32_MAX; }


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

enum { mod_err_cap = s_cache_line - 6 };
struct legion_packed mod_err
{
    uint32_t pos;
    uint16_t len;
    char str[mod_err_cap];
};

static_assert(sizeof(struct mod_err) == s_cache_line);


struct legion_packed mod_index
{
    uint32_t pos, len;
    vm_ip ip;
};

static_assert(sizeof(struct mod_index) == 12);


struct legion_packed mod_pub
{
    uint64_t key;
    vm_ip ip;
    legion_pad(4);
};

static_assert(sizeof(struct mod_pub) == 16);


struct legion_packed mod
{
    mod_id id;

    uint32_t len;
    uint32_t src_len;
    uint32_t pub_len;
    uint32_t errs_len;
    uint32_t index_len;
    uint64_t src_hash;

    char *src;
    struct mod_pub *pub;
    struct mod_err *errs;
    struct mod_index *index;
    uint8_t code[];
};

static_assert(sizeof(struct mod) == s_cache_line);


struct mod *mod_alloc(
        const char *src, size_t src_len,
        const uint8_t *code, size_t code_len,
        const struct mod_pub *pub, size_t pub_len,
        const struct mod_err *errs, size_t errs_len,
        const struct mod_index *index, size_t index_len);
void mod_free(const struct mod *);

struct mod *mod_load(struct save *);
void mod_save(const struct mod *, struct save *);

inline size_t mod_len(const struct mod *mod)
{
    return sizeof(*mod) +
        mod->len * sizeof(*mod->code) +
        mod->src_len * sizeof(*mod->src) +
        mod->pub_len * sizeof(*mod->pub) +
        mod->errs_len * sizeof(*mod->errs) +
        (mod->index_len + 1) * sizeof(*mod->index);
}

void mod_compiler_init(void);
struct mod *mod_compile(
        mod_maj, const char *src, size_t len, struct mods *, struct atoms *);

size_t mod_dump(const struct mod *, char *dst, size_t len);
size_t mod_hexdump(const struct mod *, char *dst, size_t len);

static const vm_ip MOD_PUB_UNKNOWN = -1;
vm_ip mod_pub(const struct mod *, uint64_t key);

struct mod_index mod_index(const struct mod *, vm_ip ip);
vm_ip mod_byte(const struct mod *, uint32_t pos);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mods *mods_new(void);
void mods_free(struct mods *);

struct mods *mods_load(struct save *);
void mods_save(const struct mods *, struct save *);

mod_id mods_register(struct mods *, user_id, const struct symbol *name);
bool mods_name(struct mods *, mod_maj, struct symbol *dst);
user_id mods_owner(struct mods *, mod_maj);

mod_id mods_set(struct mods *, mod_maj, const struct mod *);
const struct mod *mods_get(struct mods *, mod_id);
const struct mod *mods_latest(struct mods *, mod_maj);

mod_maj mods_find(struct mods *, const struct symbol *name);

const struct mod *mods_parse(struct mods *, const char *it, size_t len);

struct mods_item
{
    mod_maj maj;
    mod_ver ver;
    struct symbol str;
};

struct mods_list
{
    uint32_t len, cap;
    struct mods_item items[];
};

struct mods_list *mods_list(struct mods *, user_set);
struct mods_list *mods_list_reserve(size_t len);

void mods_list_save(struct mods *, struct save *, user_set);
bool mods_list_load_into(struct mods_list **, struct save *);

void mods_populate(struct mods *, struct atoms *);


// -----------------------------------------------------------------------------
// lisp
// -----------------------------------------------------------------------------

struct lisp;

struct lisp_ret
{
    bool ok;
    vm_word value;
};

struct lisp *lisp_new(struct mods_list *, struct atoms *);
void lisp_free(struct lisp *);

void lisp_context(struct lisp *, struct mods_list *, struct atoms *);
struct lisp_ret lisp_eval_const(struct lisp *, const char *src, size_t len);
