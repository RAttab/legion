/* atom.c
   Rémi Attab (remi.attab@gmail.com), 28 Nov 2020
   FreeBSD-style copyright and disclaimer apply

   \todo entire implementation is a convoluted work-around for not having str
   hash table. Need to improve
*/

#include "vm/vm.h"
#include "utils/htable.h"

// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct legion_packed atom_data
{
    uint64_t next;
    word_t word;
    atom_t str;
};

static struct
{
    struct atom_data *base, *curr, *cap;
    word_t id;
    struct htable istr;
    struct htable iword;
} atoms;


// -----------------------------------------------------------------------------
// atom
// -----------------------------------------------------------------------------

static inline uint64_t atom_hash(const atom_t *atom)
{
    const uint64_t *ptr = ((const uint64_t *) atom);
    return ptr[0] ^ ptr[1];
}

static inline bool atom_eq(const atom_t *lhs, const atom_t *rhs)
{
    return !memcmp(lhs, rhs, vm_atom_cap);
}


// -----------------------------------------------------------------------------
// vm_atoms
// -----------------------------------------------------------------------------

void vm_atoms_init(void)
{
    const size_t items = 1 << 10;
    atoms.base = alloc_cache(items * sizeof(*atoms.base));
    atoms.curr = atoms.base + 1; // htable can't index 0
    atoms.cap = atoms.base + items;
    atoms.id = 1;

    htable_reserve(&atoms.istr, items);
    htable_reserve(&atoms.iword, items);
}

static struct atom_data *atoms_find(const atom_t *atom, struct atom_data **prev)
{
    *prev = NULL;

    struct htable_ret ret = htable_get(&atoms.istr, atom_hash(atom));
    if (!ret.ok) return NULL;

    struct atom_data *data = atoms.base + ret.value;
    while (data) {
        if (likely(atom_eq(atom, &data->str))) return data;
        *prev = data;
        data = atoms.base + data->next;
    }

    assert(*prev);
    return NULL;
}

static void atoms_expand(void)
{
    if (likely(atoms.curr < atoms.cap)) return;

    size_t old = (atoms.cap - atoms.base) / sizeof(*atoms.base);
    size_t new = old * 2;

    atoms.base = realloc(atoms.base, new * sizeof(*atoms.base));
    atoms.curr = atoms.base + old;
    atoms.cap = atoms.base + new;
}

static void atoms_grow_index()
{
    size_t cap = atoms.istr.cap * 4;
    htable_reset(&atoms.istr);
    htable_reserve(&atoms.istr, cap);

    size_t len = (atoms.curr - atoms.base) / sizeof(*atoms.base);
    for (size_t i = 0; i < len; ++i) {
        struct atom_data *data = &atoms.base[i];
        data->next = 0;

        struct atom_data *prev = NULL;
        struct atom_data *ret = atoms_find(&data->str, &prev);
        assert(!ret);

        if (prev) prev->next = i;
        else {
            uint64_t hash = atom_hash(&data->str);
            struct htable_ret ret = htable_put(&atoms.istr, hash, i);
            assert(ret.ok);
        }
    }
}

static struct atom_data *atoms_insert(const atom_t *atom, word_t id)
{
    atoms_expand();

    struct atom_data *data = atoms.curr;
    atoms.curr++;
    
    data->word = id;
    memcpy(data->str, atom, vm_atom_cap);

    uint64_t hash = atom_hash(atom);
    uint64_t index = data - atoms.base;

    struct htable_ret ret = htable_try_put(&atoms.istr, hash, index);
    assert(!ret.value);

    if (!ret.ok) {
        atoms_grow_index();
        ret = htable_try_put(&atoms.istr, hash, index);
        assert(ret.ok);
    }

    htable_put(&atoms.iword, id, index);
    assert(ret.ok);

    return data;
}

static struct atom_data *atoms_chain(
        struct atom_data *prev, const atom_t *atom, word_t id)
{
    atoms_expand();

    struct atom_data *data = atoms.curr;
    atoms.curr++;
    
    data->word = id;
    memcpy(data->str, atom, vm_atom_cap);

    uint64_t index = data - atoms.base;
    prev->next = index;

    struct htable_ret ret = htable_put(&atoms.iword, id, index);
    assert(ret.ok);

    return data;
}

word_t vm_atom(const atom_t *atom)
{
    struct atom_data *prev = NULL;
    struct atom_data *data = atoms_find(atom, &prev);

    if (data) return data->word;
    if (!prev) return atoms_insert(atom, atoms.id++)->word;
    return atoms_chain(prev, atom, atoms.id++)->word;
}

bool vm_atoms_set(const atom_t *atom, word_t id)
{
    struct htable_ret ret = {0};
    ret = htable_get(&atoms.iword, id);
    if (ret.ok) return false;

    struct atom_data *prev = NULL;
    struct atom_data *data = atoms_find(atom, &prev);

    if (data) return data->word;
    if (!prev) return atoms_insert(atom, id)->word;
    return atoms_chain(prev, atom, id)->word;
}

bool vm_atoms_str(word_t id, atom_t *dst)
{
    struct htable_ret ret = htable_get(&atoms.iword, id);
    if (!ret.ok) return false;

    memcpy(dst, atoms.base[ret.value].str, vm_atom_cap);
    return true;
}
