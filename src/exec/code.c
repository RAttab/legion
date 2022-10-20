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
            "\n"                                                        \
            "// -----------------------------------------------------------------------------\n" \
            "// %s\n"                                                   \
            "// -----------------------------------------------------------------------------\n" \
            "\n", _name)                                                \

void code_file_open(struct code_file *file, const char *dir, const char *name)
{
    // I can't be bothered...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

    snprintf(file->path, sizeof(file->path), "%s/%s.h", dir, name);

#pragma GCC diagnostic pop

    file->mfile = mfilew_create_tmp(file->path, 1048576);
    file->it = file->mfile.ptr;
    file->end = file->it + file->mfile.len;

    code_file_write(file,
            "/* This file is generated by ./legion --code */\n\n");
}

void code_file_close(struct code_file *file)
{
    assert(file->it < file->end);

    size_t len = file->it - file->mfile.ptr;
    mfilew_close(&file->mfile);
    file_tmp_swap(file->path);
    file_truncate(file->path, len);
}


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
        struct code_file specs_enum, specs_value, specs_register;
        struct code_file tape;
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
// code_parse_atoms
// -----------------------------------------------------------------------------

static void code_parse_atoms(struct code_state *state, const char *path)
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

            struct symbol field = reader_symbol(in);
            uint64_t hash = symbol_hash(&field);

            if (hash == symbol_hash_c("type")) {
                struct symbol type = reader_symbol(in);
                uint64_t hash = symbol_hash(&type);
                     if (hash == symbol_hash_c("natural"))   info->type = type_natural;
                else if (hash == symbol_hash_c("synth"))     info->type = type_synth;
                else if (hash == symbol_hash_c("passive"))   info->type = type_passive;
                else if (hash == symbol_hash_c("active"))    info->type = type_active;
                else if (hash == symbol_hash_c("logistics")) info->type = type_logistics;
                else if (hash == symbol_hash_c("sys"))       info->type = type_sys;
                else reader_err(in, "unkown info type: %s", type.c);
                reader_close(in);
            }

            else if (hash == symbol_hash_c("order")) {
                struct symbol order = reader_symbol(in);
                uint64_t hash = symbol_hash(&order);
                     if (hash == symbol_hash_c("nil"))   info->order = order_nil;
                else if (hash == symbol_hash_c("first")) info->order = order_first;
                else if (hash == symbol_hash_c("last"))  info->order = order_last;
                else reader_err(in, "unkown info order: %s", order.c);
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


// -----------------------------------------------------------------------------
// code_gen_items
// -----------------------------------------------------------------------------

static void code_gen_items(struct code_state *state)
{
    int cmp(const void *lhs_, const void *rhs_) {
        const struct code_info *lhs = lhs_;
        const struct code_info *rhs = rhs_;

        if (lhs->type != rhs->type) return lhs->type - rhs->type;
        if (lhs->order != rhs->order) return lhs->order - rhs->order;
        return symbol_cmp(&lhs->name, &rhs->name);
    }
    vec_info_sort_fn(state->info, cmp);

    void write_bounds_end(enum code_type type, vm_word atom) {
        struct symbol sym_type = code_type_sym(type);
        code_file_writef(&state->files.im_bounds,
                "  items_%s_last = 0x%02lx,\n"
                "  items_%s_len = items_%s_last - items_%s_first,\n",
                sym_type.c, atom,
                sym_type.c, sym_type.c, sym_type.c);
    }

    enum code_type type = type_nil;
    for (size_t i = 0; i < state->info->len; ++i) {
        struct code_info *info = state->info->vals + i;

        info->atom = i + 1;
        struct symbol atom = symbol_concat("item-", info->name.c);
        bool ok = atoms_set(state->atoms, &atom, info->atom);
        if (!ok) errf("duplicate item: %s", atom.c);

        if (type != info->type) {
            if (type) write_bounds_end(type, info->atom);
            type = info->type;
            struct symbol sym_type = code_type_sym(type);
            code_file_write_sep(&state->files.im_enum, sym_type.c);
            code_file_write_sep(&state->files.im_register, sym_type.c);
            code_file_writef(&state->files.im_bounds,
                    "\n  items_%s_first = 0x%02lx,\n", sym_type.c, info->atom);
        }

        struct symbol sym_enum = symbol_to_enum(info->name);

        code_file_writef(&state->files.im_enum,
                "  item_%-20s = 0x%02lx,\n", sym_enum.c, info->atom);

        if (!info->config.len) {
            code_file_writef(&state->files.im_register,
                    "im_register(item_%s, \"item-%s\");\n", sym_enum.c, info->name.c);
        }
        else {
            code_file_writef(&state->files.im_register,
                    "im_register_cfg(item_%s, \"item-%s\", %s);\n",
                    sym_enum.c, info->name.c, info->config.c);
        }

        if (i == state->info->len - 1) {
            write_bounds_end(type, info->atom + 1);
        }
    }
}


// -----------------------------------------------------------------------------
// full
// -----------------------------------------------------------------------------

static void code_gen_specs(
        struct code_state *state, struct reader *in, const struct symbol *item)
{
    struct symbol item_enum = symbol_to_enum(*item);

    uint8_t seq = 0;
    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol spec = reader_symbol(in);
        struct symbol spec_enum = symbol_to_enum(spec);

        hash_val hash = symbol_hash(&spec);
        if (    hash == symbol_hash_c("lab-bits") ||
                hash == symbol_hash_c("lab-work") ||
                hash == symbol_hash_c("lab-energy"))
        {
            code_file_writef(&state->files.specs_enum,
                    "\n  spec_%s_%-20s = make_spec(item_%s, %s),",
                    item_enum.c, spec_enum.c, item_enum.c, spec_enum.c);
        }
        else {
            code_file_writef(&state->files.specs_enum,
                    "\n  spec_%s_%-20s = make_spec(item_%s, 0x%x),",
                    item_enum.c, spec_enum.c, item_enum.c, seq++);
        }


        hash_val type = reader_symbol_hash(in);

        if (type == symbol_hash_c("fn")) {
            struct symbol fn = reader_symbol(in);
            code_file_writef(&state->files.specs_register,
                    "spec_register(spec_%s_%s, %s);\n",
                    item_enum.c, spec_enum.c, fn.c);
        }

        else if (type == symbol_hash_c("var")) {
            code_file_writef(&state->files.specs_register,
                    "spec_register(spec_%s_%s, \"spec_%s_%s\");\n",
                    item_enum.c, spec_enum.c, item_enum.c, spec.c);


            code_file_writef(&state->files.specs_value,
                    "\n  im_%s_%-20s = ", item_enum.c, spec_enum.c);

            switch (reader_peek(in)) {
            case token_number: {
                code_file_writef(&state->files.specs_value, "0x%lx,", reader_word(in));
                break;
            }
            case token_atom: {
                struct symbol atom = symbol_to_enum(reader_atom_symbol(in));
                code_file_writef(&state->files.specs_value, "%s,", atom.c);
                break;
            }
            default: { reader_err(in, "unexpected token type: %s", reader_peek(in)); break; }
            }

        }

        reader_close(in);
    }

    reader_close(in);

    code_file_write(&state->files.specs_enum, "\n");
    code_file_write(&state->files.specs_value, "\n");
    code_file_write(&state->files.specs_register, "\n");
}

static void code_gen_tape(
        struct code_state *state, struct reader *in, const struct symbol *item)
{
    uint8_t work = 0;
    im_energy energy = 0;
    struct symbol host = {0};

    size_t input = 0, output = 0;
    struct symbol tape[256] = {0};

    struct symbol item_enum = symbol_to_enum(*item);

    while (!reader_peek_close(in)) {
        reader_open(in);

        struct symbol key = reader_symbol(in);
        hash_val hash = symbol_hash(&key);

        if (hash == symbol_hash_c("work")) {
            vm_word value = reader_word(in);
            if (value < 0 || value > UINT8_MAX)
                reader_err(in, "invalid work value '%lx'", value);
            work = value;
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("energy")) {
            vm_word value = reader_word(in);
            if (value < 0 || value > UINT32_MAX)
                reader_err(in, "invalid energy value '%lx'", value);
            energy = value;
            reader_close(in);
            continue;
        }

        else if (hash == symbol_hash_c("host")) {
            struct symbol value = reader_symbol(in);
            if (!atoms_get(state->atoms, &value))
                reader_err(in, "unknown host atom '%s'", value.c);
            host = symbol_to_enum(value);
            reader_close(in);
            continue;
        }

        bool is_input = hash == symbol_hash_c("in");
        bool is_output = hash == symbol_hash_c("out");
        if (!is_input && !is_output) {
            reader_err(in, "unknown field '%s'", key.c);
            reader_goto_close(in);
            continue;
        }

        while (!reader_peek_close(in)) {
            vm_word count = 1;
            struct symbol item = {0};

            if (reader_peek(in) == token_symbol)
                item = reader_symbol(in);
            else {
                reader_open(in);
                item = reader_symbol(in);
                count = reader_word(in);
                reader_close(in);
            }

            if (!atoms_get(state->atoms, &item))
                reader_err(in, "unknown atom '%s'", item.c);

            if (count < 1 || count > UINT8_MAX) {
                reader_err(in, "invalid count '%lx'", count);
                count = 0;
            }

            if (count + input + output > UINT8_MAX) {
                reader_err(in, "tape overflow: %zu + %zu + %zu", count, input, output);
                count = 0;
            }

            item = symbol_to_enum(item);
            for (size_t i = 0; i < (size_t) count; ++i) {
                if (is_input) tape[input++] = item;
                if (is_output) tape[input + output++] = item;
            }
        }

        reader_close(in);
    }
    reader_close(in);

    code_file_writef(&state->files.tape,
            "\ntape_register(item_%s, %zu, {\n"
            "  .id = item_%s\n"
            "  .host = %s\n"
            "  .work = %u\n"
            "  .energy = %u\n"
            "  .inputs = %zu\n"
            "  .outputs = %zu\n"
            "  .tape = {\n",
            item_enum.c, input + output,
            item_enum.c, host.c, work, energy, input, output);

    for (size_t i = 0; i < input + output; ++i) {
        code_file_writef(&state->files.tape,
                "    [%03zu] = %s,\n", i, tape[i].c);
    }

    code_file_write(&state->files.tape, "  }\n});\n");
}


static void code_gen_rest(struct code_state *state, const char *file)
{
    struct config config = {0};
    struct reader *in = config_read(&config, file);

    while (!reader_peek_eof(in)) {
        reader_open(in);
        struct symbol item = reader_symbol(in);

        while (!reader_peek_close(in)) {
            reader_open(in);

            struct symbol section = reader_symbol(in);
            hash_val hash = symbol_hash(&section);

            if (hash == symbol_hash_c("info"))
                reader_goto_close(in);

            else if (hash == symbol_hash_c("specs"))
                code_gen_specs(state, in, &item);

            else if (hash == symbol_hash_c("tape"))
                code_gen_tape(state, in, &item);

            else reader_err(in, "unknown section: %s", section.c);

        }

        reader_close(in);
    }

    config_close(&config);
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

    {
        code_file_open(&state.files.im_enum, state.path.out, "im_enum");
        code_file_write(&state.files.im_enum,
                "enum legion_packed item\n{\n  item_nil = 0x00,\n");

        code_file_open(&state.files.im_bounds, state.path.out, "im_bounds");
        code_file_write(&state.files.im_bounds, "enum\n{");

        code_file_open(&state.files.im_register, state.path.out, "im_register");
        code_file_write(&state.files.im_register,
                "im_register(item_nil, \"item-nil\");\n");

        code_file_open(&state.files.specs_enum, state.path.out, "specs_enum");
        code_file_write(&state.files.specs_enum, "enum legion_packed spec\n{");

        code_file_open(&state.files.specs_value, state.path.out, "specs_value");
        code_file_write(&state.files.specs_value, "enum\n{");

        code_file_open(&state.files.specs_register, state.path.out, "specs_register");

        code_file_open(&state.files.tape, state.path.out, "tape");
    }

    {
        struct dir_it *it = dir_it(state.path.in);
        while (dir_it_next(it))
            code_parse_atoms(&state, dir_it_path(it));
        dir_it_free(it);
    }

    code_gen_items(&state);

    {
        struct dir_it *it = dir_it(state.path.in);
        while (dir_it_next(it))
            code_gen_rest(&state, dir_it_path(it));
        dir_it_free(it);
    }

    {
        code_file_write(&state.files.im_enum, "};\n");
        code_file_close(&state.files.im_enum);

        code_file_write(&state.files.im_bounds, "};\n");
        code_file_close(&state.files.im_bounds);

        code_file_close(&state.files.im_register);

        code_file_write(&state.files.specs_enum, "};\n");
        code_file_close(&state.files.specs_enum);

        code_file_write(&state.files.specs_value, "};\n");
        code_file_close(&state.files.specs_value);

        code_file_close(&state.files.specs_register);

        code_file_close(&state.files.tape);
    }

    atoms_free(state.atoms);
    free(state.info);
    return true;
}
