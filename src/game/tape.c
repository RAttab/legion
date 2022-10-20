/* tape.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/tape.h"
#include "game/sys.h"
#include "utils/fs.h"
#include "utils/str.h"
#include "utils/htable.h"
#include "utils/config.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

enum item tape_id(const struct tape *tape) { return tape->id; }
enum item tape_host(const struct tape *tape) { return tape->host; }
im_energy tape_energy(const struct tape *tape) { return tape->energy; }
im_work tape_work_cap(const struct tape *tape) { return tape->work; }

size_t tape_len(const struct tape *tape)
{
    return tape->inputs + tape->work + tape->outputs;
}

struct tape_ret tape_at(const struct tape *tape, tape_it it)
{
    tape_it bound = tape->inputs;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_input,
            .item = tape->tape[it],
        };
    }

    bound += tape->work;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_work,
            .item = ITEM_NIL,
        };
    }

    bound += tape->outputs;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_output,
            .item = tape->tape[it - tape->work],
        };
    }

    return (struct tape_ret) {
        .state = tape_eof,
        .item = ITEM_NIL,
    };
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

bool tape_set_eq(const struct tape_set *lhs, const struct tape_set *rhs)
{
    for (size_t i = 0; i < array_len(lhs->s); ++i)
        if (lhs->s[i] != rhs->s[i]) return false;
    return true;
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
        struct tapes_vec *vec,
        enum item id,
        enum item host,
        im_energy energy,
        im_work work)
{
    size_t len = vec->it * sizeof(vec->tape[0]);
    struct tape *tape = calloc(1, sizeof(*tape) + len);

    *tape = (struct tape) {
        .id = id,
        .host = host,
        .energy = energy,
        .work = work,
        .inputs = vec->in,
        .outputs = vec->out
    };
    memcpy(tape->tape, vec->tape, len);

    return tape;
}

static enum item tapes_expect_item(struct reader *in, struct atoms *atoms)
{
    vm_word item = reader_atom(in, atoms);
    if (item > 0 && item < ITEM_MAX) return item;

    reader_err(in, "invalid item atom: %lx", item);
    return ITEM_NIL;
}

static void tapes_load_tape(
        enum item host, struct reader *in, struct atoms *atoms)
{
    im_work work = 0;
    im_energy energy = 0;
    enum item id = tapes_expect_item(in, atoms);

    enum tapes_type type = tapes_nil;
    struct tapes_vec vec = {0};

    while (!reader_peek_close(in)) {
        reader_open(in);
        struct symbol field = reader_symbol(in);
        uint64_t hash = symbol_hash(&field);

        if (hash == symbol_hash_c("energy")) {
            energy = reader_word(in);
            reader_close(in);
            continue;
        }
        else if (hash == symbol_hash_c("work")) {
            work = reader_word(in);
            reader_close(in);
            continue;
        }
        else if (hash == symbol_hash_c("in")) type = tapes_in;
        else if (hash == symbol_hash_c("out")) type = tapes_out;
        else {
            reader_err(in, "unknown field: %s", field.c);
            reader_close(in);
            continue;
        }

        while (!reader_peek_close(in)) {
            enum item item = tapes_expect_item(in, atoms);
            tapes_vec_push(&vec, item, type);
        }
        reader_close(in);
    }

    if (tapes.index[id]) failf("duplicate tape: id=%x, host=%x", id, host);

    assert(!tapes.index[id]);
    tapes.index[id] = tapes_vec_output(&vec, id, host, energy, work);
}

static void tapes_load_file(const char *path, struct atoms *atoms)
{
    struct config config = {0};
    struct reader *in = config_read(&config, path);

    while (!reader_peek_eof(in)) {
        reader_open(in);

        enum item host = tapes_expect_item(in, atoms);
        while (!reader_peek_close(in)) {
            reader_open(in);
            tapes_load_tape(host, in, atoms);
            reader_close(in);
        }

        reader_close(in);
    }

    config_close(&config);
}


void tapes_populate(struct atoms *atoms)
{
    char path[PATH_MAX] = {0};
    sys_path_res("tapes", path, sizeof(path));

    struct dir_it *it = dir_it(path);

    while (dir_it_next(it))
        tapes_load_file(dir_it_path(it), atoms);

    dir_it_free(it);

    for (enum item item = 0; item < ITEM_MAX; ++item)
        (void) tapes_info_for(item);
}
