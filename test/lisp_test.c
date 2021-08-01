/* lisp_test.c
   Rémi Attab (remi.attab@gmail.com), 18 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/item.h"
#include "vm/vm.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "utils/str.h"
#include "utils/log.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

struct file
{
    uint32_t row;
    const char *base, *it, *end;
};

struct file file_open(const char *path)
{
    struct file file = {0};

    int fd = open(path, O_RDONLY);
    if (fd < 0) fail_errno("file not found: %s", path);

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

    size_t len = stat.st_size;
    file.base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file.base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

    file.it = file.base;
    file.end = file.base + len;

    close(fd);
    return file;
}

void file_close(struct file *file)
{
    munmap((void *) file->base, file->end - file->base);
}

bool file_eof(struct file *file) { return file->it >= file->end; }
char file_inc(struct file *file)
{
    if (unlikely(file->it >= file->end)) return 0;
    if (*file->it == '\n') file->row++;
    file->it++;
    return *file->it;
}


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum token_type
{
    token_nil = 0,
    token_open = 1,
    token_close = 2,
    token_atom = 3,
    token_symbol = 4,
    token_reg = 5,
    token_stack = 6,
    token_number = 7,
    token_assign = 8,
};

struct token
{
    uint32_t row;
    enum token_type type;
    union { struct symbol s; word_t w; } value;
};

void token_dbg(struct token *token, const char *title)
{
    char value[symbol_cap] = {0};

    switch (token->type) {
    case token_nil: case token_open: case token_close: case token_assign:
        strcpy(value, "nil"); break;
    case token_atom: case token_symbol:
        strcpy(value, token->value.s.c); break;
    case token_reg: case token_stack: case token_number:
        str_utox(token->value.w, value, 16); break;
    default: assert(false);
    }

    dbg("%s: tok{ row=%u, type=%d, value=%s }", title, token->row, token->type, value);
}

void skip_spaces(struct file *file)
{
    while (!file_eof(file)) {

        if (likely(str_is_space(*file->it))) {
            file_inc(file);
            continue;
        }

        if (*file->it == ';') {
            while (!file_eof(file) && *file->it != '\n')
                file_inc(file);
            continue;
        }

        return;
    }
}

struct token token_next(struct file *file)
{
    struct token token = {0};

    skip_spaces(file);
    if (file_eof(file)) { token.type = token_nil; return token; }

    token.row = file->row;

    switch (*file->it) {
    case '(': { token.type = token_open; break; }
    case ')': { token.type = token_close; break; }
    case '!': { token.type = token_atom; break; }
    case '$': { token.type = token_reg; break; }
    case '#': { token.type = token_stack; break; }
    case ':': { token.type = token_assign; break; }
    case '-':
    case '0'...'9': { token.type = token_number; break; }
    default: {
        assert(symbol_char(*file->it));
        token.type = token_symbol; break;
    }
    }

    switch (token.type)
    {

    case token_open:
    case token_close:
    case token_assign: { file_inc(file); break; }

    case token_atom:
    case token_symbol: {
        const char *first = file->it;
        if (token.type == token_atom) { file_inc(file); first++; }

        while(!file_eof(file) && symbol_char(*file->it)) file_inc(file);
        size_t len = file->it - first;

        assert(len <= symbol_cap);
        token.value.s = make_symbol_len(first, len);
        break;
    }

    case token_number: {
        const char *first = file->it;
        while (!file_eof(file) && str_is_number(*file->it)) file_inc(file);
        size_t len = file->it - first;

        size_t read = 0;
        if (len > 2 && first[0] == '0' && first[1] == 'x') {
            uint64_t value = 0;
            read = str_atox(first+2, len-2, &value) + 2;
            token.value.w = value;
        }
        else read = str_atod(first, len, &token.value.w);

        assert(read == len);
        break;
    }

    case token_reg: {
        assert(!file_eof(file));
        char c = file_inc(file);
        file_inc(file);

        assert(c >= '0' && c <= '3');
        token.value.w = c - '0';
        break;
    }

    case token_stack: {
        assert(!file_eof(file));
        char c = file_inc(file);
        file_inc(file);

        assert(str_is_number(c));
        token.value.w = c - '0';
        break;
    }

    default: { break; }
    }

    return token;
}

word_t token_word(struct file *file, struct atoms *atoms)
{
    struct token token = token_next(file);
    switch (token.type) {
    case token_number: { return token.value.w; }
    case token_atom: { return atoms_atom(atoms, &token.value.s); }
    default: { token_dbg(&token, "exp.word"); assert(false); }
    }
}

void token_goto_close(struct file *file)
{
    for (size_t depth = 1; depth && !file_eof(file); file_inc(file)) {
        if (*file->it == '(') depth++;
        if (*file->it == ')') depth--;
    }
}


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

#define token_expect(file, exp)                 \
    ({                                          \
        struct token token = token_next(file);  \
        if (token.type != (exp)) {              \
            dbg("%d != %d", token.type, exp);   \
            assert(false);                      \
        }                                       \
        token;                                  \
    })

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

#define field(str)                                                      \
    ({                                                                  \
        struct symbol symbol = make_symbol_len((str), sizeof(str));     \
        const uint64_t hash = symbol_hash(&symbol);                     \
        hash;                                                           \
    })


// -----------------------------------------------------------------------------
// dsl
// -----------------------------------------------------------------------------

struct vm *read_vm(struct file *file, struct atoms *atoms)
{
    token_expect(file, token_open);

    struct vm *vm = vm_alloc(4, 6);
    while (!file_eof(file)) {
        struct token token = token_next(file);
        if (token.type == token_close) break;

        switch (token.type)
        {

        case token_reg: {
            size_t index = token.value.w;
            token_expect(file, token_assign);
            vm->regs[index] = token_word(file, atoms);
            break;
        }

        case token_stack: {
            size_t index = token.value.w;
            token_expect(file, token_assign);
            vm->stack[index] = token_word(file, atoms);
            break;
        }

        case token_symbol: {
            uint64_t key = symbol_hash(&token.value.s);
            token_expect(file, token_assign);

            word_t value = token_word(file, NULL);
            if (key == field("S")) { vm->specs.stack = value; }
            else if (key == field("C")) { vm->specs.speed = value; }
            else if (key == field("flags")) { vm->flags = value; }
            break;
        }

        default: { assert(false); }
        }

    }

    return vm;
}

const struct mod *read_mod(
        struct file *file, const struct symbol *name,
        struct mods *mods, struct atoms *atoms)
{
    token_expect(file, token_open);

    const char *first = file->it;
    token_goto_close(file);
    assert(!file_eof(file));
    size_t len = (file->it - first) - 1;

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

    token_goto_close(file);
    return NULL;
}

bool check_mod(
        struct file *file,
        const struct symbol *name,
        struct vm *vm,
        const struct mod *mod,
        struct atoms *atoms)
{
    token_expect(file, token_open);

    ip_t ret = vm_exec(vm, mod);

    bool ok = true;
    while (!file_eof(file)) {
        struct token token = token_next(file);
        if (token.type == token_close) break;

        switch (token.type)
        {

        case token_reg: {
            reg_t reg = token.value.w;
            char name[symbol_cap] = {'$', '0'+reg, 0};

            token_expect(file, token_assign);
            ok = check_u64(name, vm->regs[reg], token_word(file, atoms)) && ok;
            break;
        }

        case token_stack: {
            size_t sp = token.value.w;
            char name[symbol_cap] = {0};
            sprintf(name, "#%zu", sp);

            token_expect(file, token_assign);
            ok = check_u64(name, vm->stack[sp], token_word(file, atoms)) && ok;
            break;
        }

        case token_symbol: {
            uint64_t key = symbol_hash(&token.value.s);
            token_expect(file, token_assign);

            word_t exp = token_word(file, atoms);
            if (key == field("S")) ok = check_u64("S", vm->specs.stack, exp) && ok;
            else if (key == field("C")) ok = check_u64("C", vm->specs.speed, exp) && ok;
            else if (key == field("io")) ok = check_u64("io", vm->io, exp) && ok;
            else if (key == field("ior")) ok = check_u64("ior", vm->ior, exp) && ok;
            else if (key == field("sp")) ok = check_u64("sp", vm->sp, exp) && ok;
            else if (key == field("ret")) ok = check_u64("ret", ret, exp) && ok;
            else if (key == field("flags")) {
                ok = check_flag(FLAG_IO, vm->flags, exp) && ok;
                ok = check_flag(FLAG_SUSPENDED, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_USER, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_REG, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_STACK, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_CODE, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_MATH, vm->flags, exp) && ok;
                ok = check_flag(FLAG_FAULT_IO, vm->flags, exp) && ok;
            }

            break;
        }

        default: { assert(false); }
        }
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
    atoms_register_game(atoms);

    bool ok = true;
    struct file file = file_open(path);

    while (!file_eof(&file)) {
        struct token token = token_next(&file);
        if (token.type == token_nil) break;
        assert(token.type == token_open);

        struct symbol name = token_expect(&file, token_symbol).value.s;

        struct vm *vm = read_vm(&file, atoms);
        const struct mod *mod = read_mod(&file, &name, mods, atoms);
        if (!mod) { ok = false; token_goto_close(&file); continue; }

        ok = check_mod(&file, &name, vm, mod, atoms) && ok;
        token_expect(&file, token_close);

        vm_free(vm);
    }

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
    mod_compiler_init();

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/test/lisp", argc > 1 ? argv[1] : ".");
    return check_dir(path) ? 0 : 1;
}
