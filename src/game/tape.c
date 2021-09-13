/* tape.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/tape.h"
#include "vm/token.h"
#include "render/core.h"
#include "utils/fs.h"
#include "utils/str.h"
#include "utils/log.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct tape
{
    enum item id;
    enum item host;
    energy_t energy;
    uint8_t inputs, outputs;
    enum item tape[];
};


enum item tape_id(const struct tape *tape) { return tape->id; }
enum item tape_host(const struct tape *tape) { return tape->host; }
energy_t tape_energy(const struct tape *tape) { return tape->energy; }
size_t tape_len(const struct tape *tape) { return tape->inputs + tape->outputs; }

struct tape_ret tape_at(const struct tape *tape, tape_it_t it)
{
    struct tape_ret ret = { .state = tape_eof };

    if (it < tape->inputs) {
        ret.state = tape_input;
        ret.item = tape->tape[it];
    }
    else if (it < tape->inputs + tape->outputs) {
        ret.state = tape_output;
        ret.item = tape->tape[it];
    }

    return ret;
}


// -----------------------------------------------------------------------------
// tape_set
// -----------------------------------------------------------------------------

size_t tape_set_len(const struct tape_set *set)
{
    size_t n = 0;
    for (size_t i = 0; i < array_len(set->s); ++i)
        n += u64_pop(set->s[i]);
    return n;
}

bool tape_set_empty(const struct tape_set *set)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        if (set->s[i]) return false;
    return true;
}

bool tape_set_check(const struct tape_set *set, enum item item)
{
    return set->s[item / 64] & (1ULL << (item % 64));
}

void tape_set_put(struct tape_set *set, enum item item)
{
    set->s[item / 64] |= 1ULL << (item % 64);
}

struct tape_set tape_set_invert(struct tape_set *set)
{
    struct tape_set ret = {0};
    for (size_t i = 0; i < array_len(set->s); ++i)
        ret.s[i] = ~set->s[i];
    return ret;
}

enum item tape_set_next(const struct tape_set *set, enum item first)
{
    if (first == ITEM_MAX) return ITEM_NIL;
    first++;

    uint64_t mask = (1ULL << (first % 64)) - 1;
    for (size_t i = first / 64; i < array_len(set->s); ++i) {
        uint64_t x = set->s[i] & ~mask;
        if (x) return u64_ctz(x) + i * 64;
        mask = 0;
    }
    return ITEM_NIL;
}

void tape_set_union(struct tape_set *set, const struct tape_set *other)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        set->s[i] |= other->s[i];
}

size_t tape_set_intersect(
        const struct tape_set *set, const struct tape_set *other)
{
    size_t n = 0;
    for (size_t i = 0; i < array_len(set->s); ++i)
        n += u64_pop(set->s[i] & other->s[i]);
    return n;
}

void tape_set_save(const struct tape_set *set, struct save *save)
{
    save_write_magic(save, save_magic_tape_set);
    save_write(save, set, sizeof(*set));
    save_write_magic(save, save_magic_tape_set);
}

bool tape_set_load(struct tape_set *set, struct save *save)
{
    if (!save_read_magic(save, save_magic_tape_set)) return false;
    save_read(save, set, sizeof(*set));
    return save_read_magic(save, save_magic_tape_set);
}


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

static struct
{
    const struct tape *index[ITEM_MAX];
    struct tape_info info[ITEM_MAX];
} tapes;

const struct tape *tapes_get(enum item id)
{
    return id < ITEM_MAX ? tapes.index[id] : NULL;
}

const struct tape_info *tapes_info(enum item id)
{
    return tapes_get(id) ? &tapes.info[id] : NULL;
}

static const struct tape_info *tapes_info_for(enum item id)
{
    const struct tape *tape = tapes_get(id);
    if (!tape) return NULL;

    struct tape_info *info = &tapes.info[id];
    if (info->rank) return info;

    if (!tape->inputs) {
        info->rank = 1;
        if (id >= ITEM_NATURAL_FIRST && id < ITEM_SYNTH_FIRST)
            info->elems[id - ITEM_NATURAL_FIRST] = 1;
        return info;
    }

    for (size_t i = 0; i < tape->inputs; ++i) {
        const struct tape_info *input = tapes_info_for(tape->tape[i]);
        info->rank = legion_max(info->rank, input->rank + 1);

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i)
            info->elems[i] += input->elems[i];

        tape_set_union(&info->reqs, &input->reqs);
        tape_set_put(&info->reqs, tape->tape[i]);
    }

    return info;
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

enum tapes_type { tapes_nil = 0, tapes_in, tapes_out };

struct tapes_vec
{
    size_t it, in, out;
    enum item tape[256];
};

static void tapes_vec_push(
        struct tapes_vec *vec, enum item item, enum tapes_type type)
{
    vec->tape[vec->it] = item;
    vec->it++;

    switch (type) {
    case tapes_in: { vec->in++; break; }
    case tapes_out: { vec->out++; break; }
    default: { assert(false); }
    }
}

static struct tape *tapes_vec_output(
        struct tapes_vec *vec, enum item id, enum item host, energy_t energy)
{
    size_t len = vec->it * sizeof(vec->tape[0]);
    struct tape *tape = calloc(1, sizeof(*tape) + len);

    *tape = (struct tape) {
        .id = id,
        .host = host,
        .energy = energy,
        .inputs = vec->in,
        .outputs = vec->out
    };
    memcpy(tape->tape, vec->tape, len);

    return tape;
}

static enum item tapes_assert_item(
        struct tokenizer *tok, struct token *token, struct atoms *atoms)
{
    assert(token_assert(tok, token, token_symbol));

    word_t item = atoms_get(atoms, &token->value.s);
    if (item <= 0 || item >= ITEM_MAX) {
        token_err(tok, "invalid item atom: %s", token->value.s.c);
        assert(false);
    }

    return item;
}

static enum item tapes_expect_item(struct tokenizer *tok, struct atoms *atoms)
{
    struct token token = {0};
    return tapes_assert_item(tok, token_next(tok, &token), atoms);
}

static void tapes_load_tape(
        enum item host, struct tokenizer *tok, struct atoms *atoms)
{
    energy_t energy = 0;
    enum item id = tapes_expect_item(tok, atoms);

    enum tapes_type type = tapes_nil;
    struct tapes_vec vec = {0};
    struct token token = {0};

    while (token_next(tok, &token)->type != token_close) {
        assert(token_assert(tok, &token, token_open));

        assert(token_expect(tok, &token, token_symbol));
        uint64_t hash = symbol_hash(&token.value.s);

        if (hash == symbol_hash_c("in")) type = tapes_in;
        else if (hash == symbol_hash_c("out")) type = tapes_out;
        else if (hash == symbol_hash_c("energy")) {
            assert(token_expect(tok, &token, token_number));
            energy = token.value.w;
            assert(token_expect(tok, &token, token_close));
            continue;
        }
        else assert(false);

        while (token_next(tok, &token)->type != token_close) {
            enum item item = tapes_assert_item(tok, &token, atoms);
            tapes_vec_push(&vec, item, type);
        }
    }

    tapes.index[id] = tapes_vec_output(&vec, id, host, energy);
}

static void tapes_load_file(const char *path, struct atoms *atoms)
{
    struct mfile file = mfile_open(path);

    struct tokenizer tok = {0};
    struct token_ctx *ctx = token_init_stderr(&tok, path, file.ptr, file.len);

    struct token token = {0};
    while (token_next(&tok, &token)->type != token_nil) {
        assert(token_assert(&tok, &token, token_open));

        enum item host = tapes_expect_item(&tok, atoms);
        while (token_next(&tok, &token)->type != token_close) {
            assert(token_assert(&tok, &token, token_open));
            tapes_load_tape(host, &tok, atoms);
        }
    }

    assert(token_ctx_ok(ctx));
    token_ctx_free(ctx);
    mfile_close(&file);
}


void tapes_populate(struct atoms *atoms)
{
    char path[PATH_MAX] = {0};
    core_path_res("tapes", path, sizeof(path));

    struct dir_it *it = dir_it(path);

    while (dir_it_next(it))
        tapes_load_file(dir_it_path(it), atoms);

    dir_it_free(it);

    for (enum item item = 0; item < ITEM_MAX; ++item)
        (void) tapes_info_for(item);
}
