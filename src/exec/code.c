/* gen.c
   Rémi Attab (remi.attab@gmail.com), 18 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "utils/fs.h"
#include "utils/config.h"

// -----------------------------------------------------------------------------
// info
// -----------------------------------------------------------------------------

enum code_type
{
    type_nil = 0,
    type_natural,
    type_synth,
    type_passive,
    type_active,
    type_logistics,
    type_sys,
};

struct symbol code_type_sym(enum code_type type)
{
    switch (type)
    {
    case type_natural: { return make_symbol("natural"); }
    case type_synth: { return make_symbol("synth"); }
    case type_passive: { return make_symbol("passive"); }
    case type_active: { return make_symbol("active"); }
    case type_logistics: { return make_symbol("logistics"); }
    case type_sys: { return make_symbol("sys"); }
    case type_nil: default: { assert(false); }
    }
}

enum code_order
{
    order_first,
    order_nil,
    order_last,
};

struct code_info
{
    enum code_type type;
    enum code_order order;
    struct symbol name, config;
    vm_word atom;
};

#define vecx_type struct code_info
#define vecx_name vec_info
#define vecx_sort_fn
#include "utils/vecx.h"


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

struct code_file
{
    char path[PATH_MAX];
    struct mfilew mfile;
    char *it, *end;
};

void code_file_open(struct code_file *file, const char *dir, const char *name)
{
    // I can't be bothered...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

    snprintf(file->path, sizeof(file->path), "%s/%s.h", dir, name);

#pragma GCC diagnostic pop

    file->mfile = mfilew_create_tmp(file->path, 4096);
    file->it = file->mfile.ptr;
    file->end = file->it + file->mfile.len;
}

void code_file_close(struct code_file *file)
{
    assert(file->it < file->end);

    size_t len = file->it - file->mfile.ptr;
    mfilew_close(&file->mfile);
    file_tmp_swap(file->path);
    file_truncate(file->path, len);
}

#define code_file_write(_f, _str)                       \
    do {                                                \
        struct code_file *f = (_f);                     \
        f->it += snprintf(f->it, f->end - f->it, _str); \
        assert(f->it < f->end);                         \
    } while (false)

#define code_file_writef(_f, _fmt, ...)                                 \
    do {                                                                \
        struct code_file *f = (_f);                                     \
        f->it += snprintf(f->it, f->end - f->it, _fmt, __VA_ARGS__);    \
        assert(f->it < f->end);                                         \
    } while (false)

#define code_file_write_sep(_f, _name)                                  \
    code_file_writef(_f,                                                \
            "\n\n"                                                      \
            "// -----------------------------------------------------------------------------\n" \
            "// %s\n"                                                   \
            "// -----------------------------------------------------------------------------\n" \
            "\n", _name)                                                \


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct code_state
{
    struct {
        char in[PATH_MAX];
        char out[PATH_MAX];
    } path;

    struct atoms *atoms;
    struct vec_info *info;

    struct {
        struct code_file im_enum, im_bounds, im_register;
    } files;
};


// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

struct symbol symbol_to_enum(struct symbol sym)
{
    for (size_t i = 0; i < sym.len; ++i)
        if (sym.c[i] == '-') sym.c[i] = '_';
    return sym;
}


// -----------------------------------------------------------------------------
// atoms
// -----------------------------------------------------------------------------

static void code_parse_for_atoms(struct code_state *state, const char *path)
{
    struct config config = {0};
    struct reader *in = config_read(&config, path);

    while (!reader_peek_eof(in)) {
        reader_open(in);

        struct code_info *info = &state->info->vals[state->info->len++];
        *info = (struct code_info) {
            .name = reader_symbol(in),
            .order = order_nil,
            .atom = 0,
        };

        reader_open(in);
        reader_symbol_str(in, "info");

        while (!reader_peek_close(in)) {
            reader_open(in);

            uint64_t hash = reader_symbol_hash(in);
            if (hash == symbol_hash_c("type")) {
                uint64_t hash = reader_symbol_hash(in);
                     if (hash == symbol_hash_c("natural"))   info->type = type_natural;
                else if (hash == symbol_hash_c("synth"))     info->type = type_synth;
                else if (hash == symbol_hash_c("passive"))   info->type = type_passive;
                else if (hash == symbol_hash_c("active"))    info->type = type_active;
                else if (hash == symbol_hash_c("logistics")) info->type = type_logistics;
                else assert(false);
                reader_close(in);
            }

            else if (hash == symbol_hash_c("order")) {
                uint64_t hash = reader_symbol_hash(in);
                     if (hash == symbol_hash_c("nil"))   info->order = order_nil;
                else if (hash == symbol_hash_c("first")) info->order = order_first;
                else if (hash == symbol_hash_c("last"))  info->order = order_last;
                else assert(false);
                reader_close(in);
            }

            else if (hash == symbol_hash_c("config")) {
                info->config = reader_symbol(in);
                reader_close(in);
            }

            else reader_goto_close(in);

        }
        assert(info->type != type_nil);

        reader_close(in); // info

        // skip the rest of the object
        reader_goto_close(in);
    }

    config_close(&config);
}

static void code_atoms(struct code_state *state)
{
    struct dir_it *it = dir_it(state->path.in);
    while (dir_it_next(it))
        code_parse_for_atoms(state, dir_it_path(it));
    dir_it_free(it);

    int cmp(const void *lhs_, const void *rhs_) {
        const struct code_info *lhs = lhs_;
        const struct code_info *rhs = rhs_;

        if (lhs->type != rhs->type) return lhs->type - rhs->type;
        if (lhs->order != rhs->order) return lhs->order - rhs->order;
        return symbol_cmp(&lhs->name, &rhs->name);
    }
    vec_info_sort_fn(state->info, cmp);

    enum code_type type = type_nil;

    for (size_t i = 0; i < state->info->len; ++i) {
        struct code_info *info = state->info->vals + i;

        info->atom = i + 1;
        bool ok = atoms_set(state->atoms, &info->name, info->atom);
        assert(ok);

        if (type != info->type) {
            type = info->type;
            struct symbol sym_type = code_type_sym(type);
            code_file_write_sep(&state->files.im_enum, sym_type.c);
            code_file_write_sep(&state->files.im_register, sym_type.c);
        }

        struct symbol sym_enum = symbol_to_enum(info->name);

        code_file_writef(&state->files.im_enum, "%-20s = 0x%02lx,\n",
                sym_enum.c, info->atom);

        if (!info->config.len) {
            code_file_writef(&state->files.im_register,
                    "im_register(%s, \"%s\");\n",
                    sym_enum.c, info->name.c);
        }
        else {
            code_file_writef(&state->files.im_register,
                    "im_register_cfg(%s, \"%s\", %s);\n",
                    sym_enum.c, info->name.c, info->config.c);
        }
    }
}


// -----------------------------------------------------------------------------
// run
// -----------------------------------------------------------------------------

bool code_run(const char *path)
{
    struct code_state state = {0};
    snprintf(state.path.in, sizeof(state.path.in), "%s/res/items", path);
    snprintf(state.path.out, sizeof(state.path.out), "%s/src/gen", path);

    state.atoms = atoms_new();
    state.info = vec_info_reserve(255);

    code_file_open(&state.files.im_enum, state.path.out, "im_enum");
    code_file_open(&state.files.im_bounds, state.path.out, "im_bounds");
    code_file_open(&state.files.im_register, state.path.out, "im_register");

    code_atoms(&state);

    code_file_close(&state.files.im_enum);
    code_file_close(&state.files.im_bounds);
    code_file_close(&state.files.im_register);

    atoms_free(state.atoms);
    free(state.info);

    return true;
}
