/* mod.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct atoms;


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------
// Mods can get quite big (multiple Kb) so packing these structure tight can
// help quite a bit.

enum : size_t { mod_err_cap = sys_cache_line_len - 4 };
struct legion_packed mod_err
{
    uint32_t pos:24;
    uint8_t len;
    char str[mod_err_cap];
};

static_assert(sizeof(struct mod_err) == sys_cache_line_len);


struct legion_packed mod_index
{
    vm_ip ip:24;
    uint32_t pos:24;
    uint8_t len:8;
};

static_assert(sizeof(struct mod_index) == 7);


struct legion_packed mod_pub
{
    uint64_t key;
    vm_ip ip:24;
};

static_assert(sizeof(struct mod_pub) == 11);


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

static_assert(sizeof(struct mod) == sys_cache_line_len);


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
const struct mod *mods_get(const struct mods *, mod_id);
const struct mod *mods_latest(const struct mods *, mod_maj);

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
