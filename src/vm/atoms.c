/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply

   \todo We're kinda praying for a low collision rate on the symbol_hash
   function.
*/

#include "vm/atoms.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

struct legion_packed atom_data
{
    word_t word;
    struct symbol symbol;
};

struct atoms
{
    word_t id;
    struct atom_data *base, *it, *end;

    struct htable istr;
    struct htable iword;
};

enum { atoms_default_cap = 1 << 10 };

struct atoms *atoms_new(void)
{
    struct atoms *atoms = calloc(1, sizeof(*atoms));
    atoms->id = 1UL << 31;

    atoms->base = alloc_cache(atoms_default_cap * sizeof(*atoms->base));
    atoms->end = atoms->base + atoms_default_cap;
    atoms->it = atoms->base + 1; // htable can't index 0

    htable_reserve(&atoms->istr, atoms_default_cap);
    htable_reserve(&atoms->iword, atoms_default_cap);

    return atoms;
}

void atoms_free(struct atoms *atoms)
{
    htable_reset(&atoms->istr);
    htable_reset(&atoms->iword);
    free(atoms->base);
    free(atoms);
}

struct atoms *atoms_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_atoms)) return NULL;

    struct atoms *atoms = atoms_new();
    save_read_into(save, &atoms->id);

    size_t len = save_read_type(save, typeof(len));
    size_t cap = atoms_default_cap;
    while (cap < len) cap *= 2;

    if (atoms->base) free(atoms->base);
    atoms->base = alloc_cache(cap * sizeof(*atoms->base));
    atoms->end = atoms->base + cap;
    atoms->it = atoms->base + len;
    save_read(save, atoms->base, (atoms->it - atoms->base) * sizeof(*atoms->it));

    htable_reserve(&atoms->iword, len);
    htable_reserve(&atoms->istr, len);

    for (struct atom_data *it = atoms->base + 1; it < atoms->it; it++) {
        struct htable_ret ret = {0};
        uint64_t index = it - atoms->base;

        ret = htable_put(&atoms->iword, it->word, index);
        assert(ret.ok);
        ret = htable_put(&atoms->istr, symbol_hash(&it->symbol), index);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_atoms)) goto fail;
    return atoms;

  fail:
    atoms_free(atoms);
    return NULL;

}

void atoms_save(struct atoms *atoms, struct save *save)
{
    save_write_magic(save, save_magic_atoms);
    size_t len = atoms->it - atoms->base;

    save_write_value(save, atoms->id);
    save_write_value(save, len);
    save_write(save, atoms->base, (atoms->it - atoms->base) * sizeof(*atoms->it));

    save_write_magic(save, save_magic_atoms);
}

static uint64_t atoms_insert(
        struct atoms *atoms, const struct symbol *symbol, word_t id)
{
    if (unlikely(atoms->it == atoms->end)) {
        size_t old = atoms->end - atoms->base;
        size_t new = old * 2;

        atoms->base = realloc(atoms->base, new * sizeof(*atoms->base));
        atoms->it = atoms->base + old;
        atoms->end = atoms->base + new;
    }

    uint64_t index = atoms->it - atoms->base;
    *atoms->it = (struct atom_data) {.word = id, .symbol = *symbol };
    atoms->it++;
    return index;
}

bool atoms_set(struct atoms *atoms, const struct symbol *symbol, word_t id)
{
    uint64_t hash = symbol_hash(symbol);
    if (htable_get(&atoms->iword, id).ok) return false;
    if (htable_get(&atoms->istr, hash).ok) return false;

    struct htable_ret ret = {0};
    uint64_t index = atoms_insert(atoms, symbol, id);

    ret = htable_put(&atoms->iword, id, index);
    assert(ret.ok);
    ret = htable_put(&atoms->istr, hash, index);
    assert(ret.ok);

    return true;
}

word_t atoms_get(struct atoms *atoms, const struct symbol *symbol)
{
    uint64_t hash = symbol_hash(symbol);
    struct htable_ret ret = htable_get(&atoms->istr, hash);
    if (!ret.ok) return 0;

    struct atom_data *data = atoms->base + ret.value;
    return data->word;
}

word_t atoms_make(struct atoms *atoms, const struct symbol *symbol)
{
    uint64_t hash = symbol_hash(symbol);
    struct htable_ret ret = htable_get(&atoms->istr, hash);

    if (ret.ok) {
        struct atom_data *data = atoms->base + ret.value;
        return data->word;
    }

    word_t id = atoms->id++;
    assert(atoms_set(atoms, symbol, id));
    return id;
}

bool atoms_str(struct atoms *atoms, word_t id, struct symbol *dst)
{
    if (!id) return false;

    struct htable_ret ret = htable_get(&atoms->iword, id);
    if (!ret.ok) return false;

    memcpy(dst, &atoms->base[ret.value].symbol, sizeof(*dst));
    return true;
}

word_t atoms_parse(struct atoms *atoms, const char *it, size_t len)
{
    const char *end = it + len;
    it += str_skip_spaces(it, end - it);

    bool make = false;
    switch (*it) {
    case '!': { make = true; break; }
    case '&': { make = false; break; }
    default: { return -1; }
    }
    it++;

    struct symbol symbol = {0};
    if (!symbol_parse(it, end - it, &symbol)) return -1;

    return make ?
        atoms_make(atoms, &symbol) :
        atoms_get(atoms, &symbol);
}

struct vec64 *atoms_list(struct atoms *atoms)
{
    struct vec64 *list = vec64_reserve(atoms->iword.len);

    for (const struct htable_bucket *it = htable_next(&atoms->iword, NULL);
         it; it = htable_next(&atoms->iword, it))
    {
        list = vec64_append(list, it->key);
    }

    return list;
}
