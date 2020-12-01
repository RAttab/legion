/* mod.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "utils/ref.h"
#include "utils/text.h"


// -----------------------------------------------------------------------------
// err
// -----------------------------------------------------------------------------

enum { mod_err_cap = line_cap };
struct mod_err
{
    size_t line, col;
    char str[mod_err_cap];
};


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct legion_packed mod
{
    ref_t ref;

    mod_t id;
    legion_pad(8 - sizeof(mod_t));

    char *src;
    size_t src_len;

    struct mod_err *errs;
    size_t errs_len;

    size_t len;
    legion_pad(8);
    uint8_t code[];
};

static_assert(sizeof(struct mod) == s_cache_line);

struct mod *mod_compile(struct text *source);

struct mod *mod_alloc(
        const struct text *src,
        const uint8_t *code, size_t code_len,
        const struct mod_err *errs, size_t errs_len);

inline size_t mod_len(struct mod *mod)
{
    return sizeof(*mod) +
        mod->len * sizeof(*mod->code) +
        mod->src_len * sizeof(*mod->src) +
        mod->errs_len * sizeof(*mod->errs);
}

inline struct mod *mod_share(struct mod *mod) { return ref_share(mod); }
inline void mod_discard(struct mod *mod) { ref_discard(mod); }


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

mod_t mods_register(const atom_t *name);
bool mods_name(mod_t, atom_t *dst);

bool mods_del(mod_t);
bool mods_store(mod_t, struct mod *);
struct mod *mods_load(mod_t);

