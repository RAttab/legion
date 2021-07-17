/* mod.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "utils/ref.h"
#include "utils/text.h"

struct save;

// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

enum { mod_err_cap = text_line_cap };
struct mod_err
{
    uint32_t row, col;
    char str[mod_err_cap];
};

struct legion_packed mod_index
{
    uint32_t row, col;
    ip_t ip;
};

struct legion_packed mod
{
    mod_t id;
    legion_pad(8 - sizeof(mod_t));

    uint32_t len;
    uint32_t src_len;
    uint32_t errs_len;
    uint32_t index_len;

    legion_pad(2 * 8);

    char *src;
    struct mod_err *errs;
    struct mod_index *index;
    uint8_t code[];
};

static_assert(sizeof(struct mod) == s_cache_line);

struct mod *mod_alloc(
        const char *src, size_t src_len,
        const uint8_t *code, size_t code_len,
        const struct mod_err *errs, size_t errs_len,
        const struct mod_index *index, size_t index_len);

struct mod *mod_load(struct save *);
void mod_save(const struct mod *, struct save *);

inline size_t mod_len(const struct mod *mod)
{
    return sizeof(*mod) +
        mod->len * sizeof(*mod->code) +
        mod->src_len * sizeof(*mod->src) +
        mod->errs_len * sizeof(*mod->errs) +
        (mod->index_len + 1) * sizeof(*mod->index);
}

size_t mod_dump(struct mod *mod, char *dst, size_t len);
size_t mod_hexdump(struct mod *mod, char *dst, size_t len);

size_t mod_line(struct mod *mod, ip_t ip);
ip_t mod_byte(struct mod *mod, size_t line);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mods *mods_new(void);
void mods_free(struct mods *);

struct mods *mods_load(struct save *);
void mods_save(const struct mods *, struct save *);

mod_t mods_register(struct mods *, const atom_t *name);
bool mods_name(struct mods *, mod_id_t, atom_t *dst);

mod_t mods_set(struct mods *, mod_id_t, struct mod *);
const struct mod *mods_get(struct mods *, mod_t);
const struct mod *mods_latest(struct mods *, mod_id_t);

mod_id_t mods_find(struct mods *, const atom_t *name);

struct mods_item { mod_id_t id; atom_t str; };
struct mods_list
{
    size_t len;
    struct mods_item items[];
};
struct mods_list *mods_list(struct mods *);

void mods_populate(struct mods *);


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

void mod_compile_init(void);
struct mod *mod_compile(size_t len, const char *src, struct mods *mods);
