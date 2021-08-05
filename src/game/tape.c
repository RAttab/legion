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
    uint8_t bits;
    uint8_t inputs, outputs;
    enum item tape[];
};


enum item tape_id(const struct tape *tape) { return tape->id; }
enum item tape_host(const struct tape *tape) { return tape->host; }
uint8_t tape_bits(const struct tape *tape) { return tape->bits; }
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
// tapes
// -----------------------------------------------------------------------------

static struct
{
    const struct tape *index[ITEM_MAX];
    struct tape_stats stats[ITEM_MAX];
} tapes;

const struct tape *tapes_get(enum item id)
{
    return id < ITEM_MAX ? tapes.index[id] : NULL;
}

const struct tape_stats *tapes_stats(enum item id)
{
    return tapes_get(id) ? &tapes.stats[id] : NULL;
}

static const struct tape_stats *tapes_stats_for(enum item id)
{
    const struct tape *tape = tapes_get(id);
    if (!tape) return NULL;

    struct tape_stats *stats = &tapes.stats[id];
    if (stats->rank) return stats;

    if (!tape->inputs) {
        stats->rank = 1;
        if (id >= ITEM_NATURAL_FIRST && id < ITEM_SYNTH_FIRST)
            stats->elems[id - ITEM_NATURAL_FIRST] = 1;
        return stats;
    }

    for (size_t i = 0; i < tape->inputs; ++i) {
        const struct tape_stats *input = tapes_stats_for(tape->tape[i]);
        stats->rank = legion_max(stats->rank, input->rank + 1);

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i)
            stats->elems[i] += input->elems[i];

        tape_set_union(&stats->reqs, &input->reqs);
        tape_set_put(&stats->reqs, tape->tape[i]);
    }

    return stats;
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

#include <unistd.h>


struct tapes_ctx
{
    bool ok;
    const char *path;
    struct tokenizer *tok;
};

legion_printf(2, 3)
static void tapes_err(void *_ctx, const char *fmt, ...)
{
    struct tapes_ctx *ctx = _ctx;
    ctx->ok = false;

    char str[256] = {0};
    char *it = str;
    char *end = str + sizeof(str);

    it += snprintf(str, end - it, "%s:%zu:%zu: ",
            ctx->path, ctx->tok->row+1, ctx->tok->col+1);

    va_list args;
    va_start(args, fmt);
    it += vsnprintf(it, end - it, fmt, args);
    va_end(args);

    *it = '\n'; it++;

    write(2, str, it - str);
}


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
        struct tapes_vec *vec, enum item id, enum item host, uint8_t bits)
{
    size_t len = vec->it * sizeof(vec->tape[0]);
    struct tape *tape = calloc(1, sizeof(*tape) + len);

    *tape = (struct tape) {
        .id = id,
        .host = host,
        .bits = bits,
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
    word_t item = atoms_atom(atoms, &token->value.s);

    if (item <= 0 || item >= ITEM_MAX) {
        tapes_err(tok->err_ctx, "invalid item atom: %s", token->value.s.c);
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
    enum item id = tapes_expect_item(tok, atoms);

    uint8_t bits = 0;
    enum tapes_type type = tapes_nil;
    struct tapes_vec vec = {0};
    struct token token = {0};

    while (token_next(tok, &token)->type != token_close) {
        assert(token_assert(tok, &token, token_open));

        assert(token_expect(tok, &token, token_symbol));
        uint64_t hash = symbol_hash(&token.value.s);

        if (hash == symbol_hash_c("bits")) {
            token_expect(tok, &token, token_number);
            assert(token.value.w > 0 && token.value.w <= 64);
            bits = token.value.w;

            token_expect(tok, &token, token_close);
            continue;
        }

        if (hash == symbol_hash_c("in")) type = tapes_in;
        else if (hash == symbol_hash_c("out")) type = tapes_out;
        else assert(false);

        while (token_next(tok, &token)->type != token_close) {
            enum item item = tapes_assert_item(tok, &token, atoms);
            tapes_vec_push(&vec, item, type);
        }
    }

    tapes.index[id] = tapes_vec_output(&vec, id, host, bits);
}

static void tapes_load_file(const char *path, struct atoms *atoms)
{
    struct mfile file = mfile_open(path);

    struct tokenizer tok = {0};
    struct tapes_ctx ctx = { .ok = true, .path = path, .tok = &tok };
    token_init(&tok, file.ptr, file.len, tapes_err, &ctx);

    struct token token = {0};
    while (token_next(&tok, &token)->type != token_nil) {
        assert(token_assert(&tok, &token, token_open));

        enum item host = tapes_expect_item(&tok, atoms);
        while (token_next(&tok, &token)->type != token_close) {
            assert(token_assert(&tok, &token, token_open));
            tapes_load_tape(host, &tok, atoms);
        }
    }

    mfile_close(&file);
    assert(ctx.ok);
}


void tapes_populate(void)
{
    struct atoms *atoms = atoms_new();
    atoms_register_game(atoms);

    char path[PATH_MAX] = {0};
    core_path_res("tapes", path, sizeof(path));

    struct dir_it *it = dir_it(path);

    while (dir_it_next(it))
        tapes_load_file(dir_it_path(it), atoms);

    dir_it_free(it);

    for (enum item item = 0; item < ITEM_MAX; ++item)
        (void) tapes_stats_for(item);
}
