/* lisp_test.c
   Rémi Attab (remi.attab@gmail.com), 18 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/vm.h"
#include "vm/mod.h"
#include "vm/token.h"
#include "vm/atoms.h"
#include "items/item.h"
#include "items/config.h"
#include "utils/str.h"
#include "utils/log.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

struct file
{
    const char *path;
    struct tokenizer tok;
};

legion_printf(2, 3)
void file_err(void *ctx, const char *fmt, ...)
{
    struct file *file = ctx;

    char str[256] = {0};
    char *it = str;
    char *end = str + sizeof(str);

    it += snprintf(str, end - it, "%s:%zu:%zu: ",
            file->path, file->tok.row+1, file->tok.col+1);

    va_list args;
    va_start(args, fmt);
    it += vsnprintf(it, end - it, fmt, args);
    va_end(args);

    *it = '\n'; it++;

    write(2, str, it - str);
}

struct file *file_open(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) fail_errno("file not found: %s", path);

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

    size_t len = stat.st_size;
    const char *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

    struct file *file = calloc(1, sizeof(*file));
    file->path = path;

    token_init(&file->tok, base, len, file_err, file);

    close(fd);
    return file;
}

void file_close(struct file *file)
{
    munmap((void *) file->tok.base, file->tok.end - file->tok.base);
    free(file);
}

word_t token_word(struct tokenizer *tok, struct atoms *atoms)
{
    struct token token = {0};
    switch (token_next(tok, &token)->type) {
    case token_number: { return token.value.w; }
    case token_atom: { return atoms_atom(atoms, &token.value.s); }
    default: { token_assert(tok, &token, token_number); assert(false); }
    }
}

// -----------------------------------------------------------------------------
// field
// -----------------------------------------------------------------------------

enum field_type
{
    field_nil = 0,
    field_clock,
    field_stack,
    field_io,
    field_ior,
    field_ip,
    field_sp,
    field_si,
    field_reg,
    field_ret,
    field_flags,
};

struct field
{
    enum field_type type;
    uint8_t index;
    word_t value;
    char name[8];
};


struct field token_field(struct tokenizer *tok, struct atoms *atoms)
{
    struct token token = {0};

    if (token_peek(tok, &token)->type == token_close)
        return (struct field) { .type = field_nil };
    assert(token_expect(tok, &token, token_open));

    assert(token_expect(tok, &token, token_symbol));
    uint64_t hash = symbol_hash(&token.value.s);

    struct field field = {0};

    void set(enum field_type type, const char *name)
    {
        field.type = type;
        strcpy(field.name, name);
    }

    if (hash == symbol_hash_c("C")) set(field_clock, "speed");
    else if (hash == symbol_hash_c("S")) set(field_stack, "stack");
    else if (hash == symbol_hash_c("io")) set(field_io, "io");
    else if (hash == symbol_hash_c("ior")) set(field_ior, "ior");
    else if (hash == symbol_hash_c("ip")) set(field_ip, "ip");
    else if (hash == symbol_hash_c("sp")) set(field_sp, "sp");
    else if (hash == symbol_hash_c("s")) field.type = field_si;
    else if (hash == symbol_hash_c("r")) field.type = field_reg;
    else if (hash == symbol_hash_c("ret")) set(field_ret, "ret");
    else if (hash == symbol_hash_c("flags")) set(field_flags, "flags");
    else { assert(false); }

    if (field.type == field_si || field.type == field_reg) {
        assert(token_expect(tok, &token, token_number));
        assert(token.value.w < 16);
        field.index = token.value.w;

        field.name[0] = field.type == field_si ? '#' : '$';
        field.name[1] = str_hexchar(field.index);
        field.name[2] = 0;
    }

    field.value = token_word(tok, atoms);

    assert(token_expect(tok, &token, token_close));
    return field;
}


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

#define check_u64(field, _tval_, _texp_)                                \
    ({                                                                  \
        uint64_t _val_ = (_tval_);                                      \
        uint64_t _exp_ = (_texp_);                                      \
        bool ok = _val_ == _exp_;                                       \
        if (!ok) dbg("<%s> val:%lx != exp:%lx", field, _val_, _exp_);   \
        ok;                                                             \
    })


#define check_flag(flag, _tval_, _texp_)                                \
    ({                                                                  \
        uint8_t _val_ = (_tval_) & (flag);                              \
        uint8_t _exp_ = (_texp_) & (flag);                              \
        bool ok = _val_ == _exp_;                                       \
        if (!ok) dbg("<%s> val:%x != exp:%x", #flag, _val_, _exp_);     \
        ok;                                                             \
    })


// -----------------------------------------------------------------------------
// dsl
// -----------------------------------------------------------------------------

void setup_vm(struct tokenizer *tok, struct vm *vm, struct atoms *atoms)
{
    struct field field = {0};
    while ((field = token_field(tok, atoms)).type != field_nil) {
        switch (field.type) {
        case field_clock: { vm->specs.speed = field.value; break; }
        case field_stack: { vm->specs.stack = field.value; break; }
        case field_reg: { vm->regs[field.index] = field.value; break; }
        case field_flags: { vm->flags = field.value; break; }
        default: { assert(false); }
        }
    }
}

const struct mod *read_mod(
        struct tokenizer *tok, const struct symbol *name,
        struct mods *mods, struct atoms *atoms)
{
    const char *first = tok->it;
    token_goto_close(tok);
    assert(!token_eof(tok));
    size_t len = (tok->it - first) - 1;

    mod_t id = mods_register(mods, name);
    assert(id);

    const struct mod *mod = mod_compile(mod_maj(id), first, len, mods, atoms);
    if (mod && !mod->errs_len) {
        mods_set(mods, mod_maj(id), mod);
        return mod;
    }

    for (size_t i = 0; i < mod->errs_len; ++i) {
        const struct mod_err *err = mod->errs + i;
        dbg("%u:%u:%u: %s", err->row, err->col, err->len, err->str);
    }

    return NULL;
}

bool check_mod(
        struct tokenizer *tok,
        const struct symbol *name,
        struct vm *vm,
        const struct mod *mod,
        struct atoms *atoms)
{
    if (!vm) return false;
    ip_t ret = vm_exec(vm, mod);


    bool ok = true;
    struct field field = {0};
    struct field flags = {0};
    while ((field = token_field(tok, atoms)).type != field_nil) {

        word_t exp = field.value;
        const char *str = field.name;

        switch (field.type)
        {
        case field_clock: { ok = check_u64(str, vm->specs.speed, exp) && ok; break; }
        case field_stack: { ok = check_u64(str, vm->specs.stack, exp) && ok; break; }
        case field_io:    { ok = check_u64(str, vm->io, exp) && ok; break; }
        case field_ior:   { ok = check_u64(str, vm->ior, exp) && ok; break; }
        case field_ip:    { ok = check_u64(str, vm->ip, exp) && ok; break; }
        case field_sp:    { ok = check_u64(str, vm->sp, exp) && ok; break; }
        case field_si:    { ok = check_u64(str, vm->stack[field.index], exp) && ok; break; }
        case field_reg:   { ok = check_u64(str, vm->regs[field.index], exp) && ok; break; }
        case field_ret:   { ok = check_u64(str, ret, exp) && ok; break; }
        case field_flags: { flags = field; break; }
        default: { assert(false); }
        }
    }

    {
        word_t exp = flags.type == field_flags ? flags.value : 0;
        ok = check_flag(FLAG_IO, vm->flags, exp) && ok;
        ok = check_flag(FLAG_SUSPENDED, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_USER, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_REG, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_STACK, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_CODE, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_MATH, vm->flags, exp) && ok;
        ok = check_flag(FLAG_FAULT_IO, vm->flags, exp) && ok;
    }


    if (!ok) {
        char title[80] = {0};
        ssize_t len = snprintf(title, sizeof(title), "[ %s ]", name->c);
        memset(title + len, '=', sizeof(title) - len - 1);
        dbg("%s", title);

        char buffer[s_page_len] = {0};
        dbg("\n<src>\n%s\n", mod->src);

        mod_dump(mod, buffer, sizeof(buffer));
        dbg("<bytecode:%x:%u>\n%s\n", (unsigned) mod->id, mod->len, buffer);

        vm_dbg(vm, buffer, sizeof(buffer));
        dbg("<ret>\n%sret:   %x\n", buffer, ret);
    }

    return ok;
}

bool check_file(const char *path)
{
    struct mods *mods = mods_new();
    struct atoms *atoms = atoms_new();
    im_populate_atoms(atoms);

    struct file *file = file_open(path);

    bool ok = true;
    struct token token = {0};
    struct tokenizer *tok = &file->tok;

    while (!token_eof(tok)) {

        token_next(tok, &token);
        if (token.type == token_nil) break;
        assert(token_assert(tok, &token, token_open));

        const struct mod *mod = NULL;
        struct vm *vm = vm_alloc(4, 6);

        assert(token_expect(tok, &token, token_symbol));
        struct symbol name = token.value.s;

        while (token_next(tok, &token)->type != token_close) {
            assert(token_assert(tok, &token, token_open));

            assert(token_expect(tok, &token, token_symbol));
            uint64_t hash = symbol_hash(&token.value.s);

            if (hash == symbol_hash_c("vm")) {
                setup_vm(tok, vm, atoms);
                assert(token_expect(tok, &token, token_close));
            }
            else if (hash == symbol_hash_c("mod")) {
                mod = read_mod(tok, &name, mods, atoms);
            }
            else if (hash == symbol_hash_c("check")) {
                ok = check_mod(tok, &name, vm, mod, atoms) && ok;
                assert(token_expect(tok, &token, token_close));
            }
            else assert(false);

        }

        vm_free(vm);
    }

    file_close(file);
    mods_free(mods);
    atoms_free(atoms);
    return ok;
}

bool check_dir(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) fail_errno("can't open dir: %s", path);

    bool ok = true;

    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        char file[PATH_MAX];
        snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);

        ok = check_file(file) && ok;
    }
    closedir(dir);

    return ok;
}


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    im_populate();
    mod_compiler_init();

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/test/lisp", argc > 1 ? argv[1] : ".");
    return check_dir(path) ? 0 : 1;
}
