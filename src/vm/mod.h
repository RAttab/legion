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
// mod
// -----------------------------------------------------------------------------

enum { mod_err_cap = text_line_cap };
struct mod_err
{
    size_t line;
    char str[mod_err_cap];
};

struct legion_packed mod_index
{
    uint16_t line;
    addr_t byte;
};

struct legion_packed mod
{
    ref_t ref;

    mod_t id;
    legion_pad(8 - sizeof(mod_t));

    uint32_t len;
    uint32_t src_len;
    uint32_t errs_len;
    uint32_t index_len;

    char *src;
    struct mod_err *errs;
    struct mod_index *index;
    legion_pad(8);
    uint8_t code[];
};

static_assert(sizeof(struct mod) == s_cache_line);

void vm_compile_init(void);
struct mod *mod_compile(struct text *source);

struct mod *mod_alloc(
        const struct text *src,
        const uint8_t *code, size_t code_len,
        const struct mod_err *errs, size_t errs_len,
        const struct mod_index *index, size_t index_len);

inline size_t mod_len(struct mod *mod)
{
    return sizeof(*mod) +
        mod->len * sizeof(*mod->code) +
        mod->src_len * sizeof(*mod->src) +
        mod->errs_len * sizeof(*mod->errs) +
        (mod->index_len + 1) * sizeof(*mod->index);
}

inline struct mod *mod_share(struct mod *mod) { return ref_share(mod); }
inline void mod_discard(struct mod *mod) { ref_discard(mod); }

size_t mod_dump(struct mod *mod, char *dst, size_t len);
size_t mod_hexdump(struct mod *mod, char *dst, size_t len);

size_t mod_line(struct mod *mod, ip_t ip);
addr_t mod_byte(struct mod *mod, size_t line);


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

mod_t mods_register(const atom_t *name);
bool mods_name(mod_t, atom_t *dst);
void mods_free();

bool mods_del(mod_t);
bool mods_store(mod_t, struct mod *);
mod_t mods_find(const atom_t *name);
struct mod *mods_load(mod_t);

struct mods_item { mod_t id; atom_t str; };
struct mods
{
    size_t len;
    struct mods_item items[];
};
struct mods *mods_list(void);

void mods_preload(void);
