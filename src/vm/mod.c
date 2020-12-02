/* db.c
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "utils/htable.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct mod *mod_alloc(
        const struct text *src,
        const uint8_t *code, size_t code_len,
        const struct mod_err *errs, size_t errs_len)
{
    size_t head_bytes = sizeof(struct mod);
    size_t src_bytes = src->len * line_cap;
    size_t code_bytes = code_len * sizeof(*code);
    size_t errs_bytes = errs_len * sizeof(*errs);
    size_t total_bytes = align_cache(head_bytes + code_bytes + src_bytes + errs_bytes);

    struct mod *mod = alloc_cache(total_bytes);

    memcpy(&mod->code, code, code_bytes);
    mod->len = code_len;

    mod->src = ((void *) mod->code) + code_bytes;
    text_pack(src, mod->src, src_bytes);
    mod->src_len = src_bytes;

    mod->errs = ((void *) mod->src) + src_bytes;
    memcpy(mod->errs, errs, errs_bytes);
    mod->errs_len = errs_len;

    return mod;
}


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mods_entry
{
    mod_t id;
    atom_t str;
    struct mod *mod;
};

static struct
{
    mod_t ids;
    struct htable index;
} mods;


mod_t mods_register(const atom_t *name)
{
    struct mods_entry *entry = calloc(1, sizeof(*entry));
    memcpy(entry->str, name, vm_atom_cap);
    entry->id = ++mods.ids;
    assert(entry->id);

    struct htable_ret ret = htable_put(&mods.index, entry->id, (uint64_t) entry);
    assert(ret.ok);

    return entry->id;
}

bool mods_name(mod_t id, atom_t *dst)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    memcpy(dst, entry->str, vm_atom_cap);
    return true;
}

bool mods_del(mod_t id)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    if (entry->mod) mod_discard(entry->mod);

    ret = htable_del(&mods.index, id);
    assert(ret.ok);
    
    return true;
}

bool mods_store(mod_t id, struct mod *mod)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    if (entry->mod) mod_discard(entry->mod);
    entry->mod = mod_share(mod);
    mod->id = id;

    return true;
}

struct mod *mods_load(mod_t id)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return NULL;

    struct mods_entry *entry = (void *) ret.value;
    return mod_share(entry->mod);
}
