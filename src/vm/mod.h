/* mod.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/types.h"

struct mods;
struct save;
struct text;


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

enum { mod_err_cap = s_cache_line - 8 };
struct legion_packed mod_err
{
    uint32_t row;
    uint16_t col;
    uint16_t len;

    char str[mod_err_cap];
};

static_assert(sizeof(struct mod_err) == s_cache_line);


struct legion_packed mod_index
{
    uint32_t row, col, len;
    ip_t ip;
};

static_assert(sizeof(struct mod_index) == 16);


struct legion_packed mod_pub
{
    uint64_t key;
    ip_t ip;
    legion_pad(4);
};

static_assert(sizeof(struct mod_pub) == 16);


struct legion_packed mod
{
    mod_t id;

    uint32_t len;
    uint32_t src_len;
    uint32_t pub_len;
    uint32_t errs_len;
    uint32_t index_len;

    legion_pad(8);

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
struct mod *mod_compile(size_t len, const char *src, struct mods *, struct atoms *);
struct text mod_disasm(const struct mod *);

size_t mod_dump(const struct mod *, char *dst, size_t len);
size_t mod_hexdump(const struct mod *, char *dst, size_t len);

ip_t mod_pub(const struct mod *, uint64_t key);

struct mod_index mod_index(const struct mod *, ip_t ip);
ip_t mod_byte(const struct mod *, size_t row, size_t col);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mods *mods_new(void);
void mods_free(struct mods *);

struct mods *mods_load(struct save *);
void mods_save(const struct mods *, struct save *);

mod_t mods_register(struct mods *, const struct symbol *name);
bool mods_name(struct mods *, mod_id_t, struct symbol *dst);

mod_t mods_set(struct mods *, mod_id_t, const struct mod *);
const struct mod *mods_get(struct mods *, mod_t);
const struct mod *mods_latest(struct mods *, mod_id_t);

mod_id_t mods_find(struct mods *, const struct symbol *name);

struct mods_item { mod_id_t id; struct symbol str; };
struct mods_list
{
    size_t len;
    struct mods_item items[];
};
struct mods_list *mods_list(struct mods *);

void mods_populate(struct mods *, struct atoms *);
