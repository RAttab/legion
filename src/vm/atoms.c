/* atoms.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply

   \todo entire implementation is a convoluted work-around for not having str
   hash table. Need to improve
*/

#include "vm/types.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

struct legion_packed atom_data
{
    uint64_t next;
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

enum
{
    atoms_default_cap = 1 << 10,

    atoms_cap_mul = 2,
    atoms_cap_str_mul = 4,
};

struct atoms *atoms_new(void)
{
    struct atoms *atoms = calloc(1, sizeof(*atoms));
    atoms->id = 1;

    atoms->base = alloc_cache(atoms_default_cap * sizeof(*atoms->base));
    atoms->end = atoms->base + atoms_default_cap;
    atoms->it = atoms->base + atoms->id; // htable can't index 0

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
    size_t cap_str = atoms_default_cap;
    while (cap < len) {
        cap *= atoms_cap_mul;
        cap_str *= atoms_cap_str_mul;
    }

    atoms->base = alloc_cache(cap * sizeof(*atoms->base));
    atoms->end = atoms->base + cap;
    atoms->it = atoms->base + len;
    save_read(save, atoms->base, atoms->it - atoms->base);


    htable_reserve(&atoms->iword, cap);
    htable_reserve(&atoms->istr, cap_str);

    for (struct atom_data *it = atoms->base; it < atoms->end; it++) {
        it->next = 0;
        uint64_t index = (it - atoms->base) / sizeof(*it);

        struct htable_ret ret = htable_put(&atoms->iword, it->word, index);
        assert(ret.ok);

        ret = htable_try_put(&atoms->istr, symbol_hash(&it->symbol), index);
        assert(ret.ok || ret.value);

        if (!ret.ok) {
            struct atom_data *prev = atoms->base + index;
            prev->next = index;
        }
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
    size_t len = (atoms->it - atoms->base) / sizeof(*atoms->it);

    save_write_value(save, atoms->id);
    save_write_value(save, len);
    save_write(save, atoms->base, atoms->it - atoms->base);

    save_write_magic(save, save_magic_atoms);
}


static struct atom_data *atoms_find(
        struct atoms *atoms, const struct symbol *symbol, struct atom_data **prev)
{
    *prev = NULL;

    struct htable_ret ret = htable_get(&atoms->istr, symbol_hash(symbol));
    if (!ret.ok) return NULL;

    struct atom_data *data = atoms->base + ret.value;
    while (data) {
        if (likely(symbol_eq(symbol, &data->symbol))) return data;
        *prev = data;
        data = atoms->base + data->next;
    }

    assert(*prev);
    return NULL;
}

static void atoms_expand(struct atoms *atoms)
{
    if (likely(atoms->it < atoms->end)) return;

    size_t old = (atoms->end - atoms->base) / sizeof(*atoms->base);
    size_t new = old * atoms_cap_mul;

    atoms->base = realloc(atoms->base, new * sizeof(*atoms->base));
    atoms->it = atoms->base + old;
    atoms->end = atoms->base + new;
}

static void atoms_grow_index(struct atoms *atoms)
{
    size_t end = atoms->istr.len * atoms_cap_str_mul;
    htable_reset(&atoms->istr);
    htable_reserve(&atoms->istr, end);

    size_t len = (atoms->it - atoms->base) / sizeof(*atoms->base);
    for (size_t i = 0; i < len; ++i) {
        struct atom_data *data = &atoms->base[i];
        data->next = 0;

        struct atom_data *prev = NULL;
        struct atom_data *ret = atoms_find(atoms, &data->symbol, &prev);
        assert(!ret);

        if (prev) prev->next = i;
        else {
            uint64_t hash = symbol_hash(&data->symbol);
            struct htable_ret ret = htable_put(&atoms->istr, hash, i);
            assert(ret.ok);
        }
    }
}

static struct atom_data *atoms_insert(
        struct atoms *atoms, const struct symbol *symbol, word_t id)
{
    atoms_expand(atoms);

    struct atom_data *data = atoms->it;
    atoms->it++;

    data->word = id;
    data->symbol = *symbol;

    uint64_t hash = symbol_hash(symbol);
    uint64_t index = data - atoms->base;

    struct htable_ret ret = htable_try_put(&atoms->istr, hash, index);
    assert(!ret.value);

    if (!ret.ok) {
        atoms_grow_index(atoms);
        ret = htable_try_put(&atoms->istr, hash, index);
        assert(ret.ok);
    }

    htable_put(&atoms->iword, id, index);
    assert(ret.ok);

    return data;
}

static struct atom_data *atoms_chain(
        struct atoms *atoms, struct atom_data *prev,
        const struct symbol *symbol, word_t id)
{
    atoms_expand(atoms);

    struct atom_data *data = atoms->it;
    atoms->it++;

    data->word = id;
    data->symbol = *symbol;

    uint64_t index = data - atoms->base;
    prev->next = index;

    struct htable_ret ret = htable_put(&atoms->iword, id, index);
    assert(ret.ok);

    return data;
}

bool atoms_set(struct atoms *atoms, const struct symbol *symbol, word_t id)
{
    struct htable_ret ret = {0};
    ret = htable_get(&atoms->iword, id);
    if (ret.ok) return false;

    struct atom_data *prev = NULL;
    struct atom_data *data = atoms_find(atoms, symbol, &prev);

    if (data) return data->word;
    if (!prev) return atoms_insert(atoms,symbol, id)->word;
    return atoms_chain(atoms, prev, symbol, id)->word;
}

word_t atoms_atom(struct atoms *atoms, const struct symbol *symbol)
{
    struct atom_data *prev = NULL;
    struct atom_data *data = atoms_find(atoms, symbol, &prev);

    if (data) return data->word;
    if (!prev) return atoms_insert(atoms, symbol, atoms->id++)->word;
    return atoms_chain(atoms, prev, symbol, atoms->id++)->word;
}

bool atoms_str(struct atoms *atoms, word_t id, struct symbol *dst)
{
    struct htable_ret ret = htable_get(&atoms->iword, id);
    if (!ret.ok) return false;

    memcpy(dst, &atoms->base[ret.value].symbol, sizeof(*dst));
    return true;
}
