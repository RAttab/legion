/* lisp_test.c
   Rémi Attab (remi.attab@gmail.com), 18 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/item.h"
#include "vm/mod.h"
#include "vm/vm.h"
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
    char c = *file->it;
    file->it++;
    return c;
}


// -----------------------------------------------------------------------------
// token
// -----------------------------------------------------------------------------

enum token_type
{
    token_nil = 0,
    token_open,
    token_close,
    token_atom,
    token_symbol,
    token_reg,
    token_stack,
    token_number,
    token_assign,
};

struct token
{
    uint32_t row;
    enum token_type type;
    union { struct symbol s; word_t w; } value;
};

bool is_space(char c) { return c <= 0x20; }

bool is_symbol(char c)
{
    switch (c) {
    case '-':
    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9': return true;
    default: return false;
    }
}
bool is_number(char c)
{
    switch (c) {
    case '-':
    case 'x':
    case '0'...'9': return true;
    default: return false;
    }
}

void skip_spaces(struct file *file)
{
    while (!file_eof(file)) {

        if (likely(is_space(*file->it))) {
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
    case '&': { token.type = token_stack; break; }
    case ':': { token.type = token_assign; break; }
    case '0'...'9': { token.type = token_number; break; }
    default: {
        assert(is_symbol(*file->it));
        token.type = token_symbol; break;
    }
    }

    switch (token.type)
    {

    case token_atom:
    case token_symbol: {
        const char *first = file->it;
        while(!file_eof(file) && is_symbol(*file->it)) file_inc(file);
        size_t len = file->it - first;

        assert(len <= symbol_cap);
        token.value.s = make_symbol_len(len, first);
        break;
    }

    case token_number: {
        const char *first = file->it;
        while (!file_eof(file) && is_number(*file->it)) file_inc(file);
        size_t len = file->it - first;

        size_t read = 0;
        if (len > 2 && first[0] == '0' && first[1] == 'x') {
            uint64_t value = 0;
            read = str_atox(file->it+2, len-2, &value) + 2;
            token.value.w = value;
        }
        else read = str_atod(file->it, len, &token.value.w);

        assert(read == len);
        break;
    }

    case token_reg: {
        assert(!file_eof(file));
        char c = file_inc(file);
        assert(c >= '0' && c <= '3');
        token.value.w = c - '0';
        break;
    }

    case token_stack: {
        assert(!file_eof(file));
        char c = file_inc(file);
        assert(is_number(c));
        token.value.w = c - '0';
        break;
    }

    default: { break; }
    }

    file_inc(file);
    return token;
}

struct token token_expect(struct file *file, enum token_type exp)
{
    struct token token = token_next(file);
    assert(token.type == exp);
    return token;
}


word_t token_word(struct file *file, struct atoms *atoms)
{
    struct token token = token_next(file);
    switch (token.type) {
    case token_number: { return token.value.w; }
    case token_atom: { return atoms_atom(atoms, &token.value.s); }
    default: { assert(false); }
    }
}

void token_goto_close(struct file *file)
{
    for (size_t depth = 1; depth && !file_eof(file); file_inc(file)) {
        if (*file->it == '(') depth++;
        if (*file->it == ')') depth--;
    }
    if (!file_eof(file)) assert(file_inc(file) == ')');
}


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

#define check_u64(field, _tval_, _texp_)                                \
    ({                                                                  \
        uint64_t _val_ = (_tval_);                                      \
        uint64_t _exp_ = (_texp_);                                      \
        bool ok = _val_ == _exp_;                                       \
        if (!ok) dbg("%s> val:%lx != exp:%lx\n", field, _val_, _exp_);  \
        ok;                                                             \
    })

#define field(str)                                                      \
    ({                                                                  \
        struct symbol symbol = make_symbol_len(sizeof(str), (str));     \
        const uint64_t hash = symbol_hash(&symbol);                     \
        hash;                                                           \
    })


// -----------------------------------------------------------------------------
// dsl
// -----------------------------------------------------------------------------

void print_title(struct file *file)
{
    struct token token = token_expect(file, token_symbol);

    char title[80];
    memset(title, '=', sizeof(title));
    title[sizeof(title) - 1] = 0;

    size_t len = snprintf(title, sizeof(title), "[ %s ]", token.value.s.c);
    title[len] = '=';
    dbg("%s\n", title);
}

struct vm *read_vm(struct file *file)
{
    token_expect(file, token_open);

    size_t stack = 0, clock = 0;

    struct token token = {0};
    while ((token = token_next(file)).type != token_symbol) {
        uint64_t key = symbol_hash(&token.value.s);
        token_expect(file, token_assign);

        word_t value = token_word(file, NULL);
        if (key == field("S")) stack = value;
        if (key == field("C")) stack = value;
    }

    assert(token.type == token_close);
    return vm_alloc(stack, clock);
}

const struct mod *read_mod(struct file *file, struct mods *mods, struct atoms *atoms)
{
    token_expect(file, token_open);

    const char *first = file->it;
    token_goto_close(file);
    assert(!file_eof(file));

    const struct mod *mod = mod_compile(file->it - first, first, mods, atoms);
    if (mod && !mod->errs_len) return mod;

    for (size_t i = 0; i < mod->errs_len; ++i) {
        const struct mod_err *err = mod->errs + i;
        dbg("%u:%u:%u: %s\n", err->row, err->col, err->len, err->str);
    }

    token_goto_close(file);
    return NULL;
}

bool check_mod(
        struct file *file,
        struct vm *vm,
        const struct mod *mod,
        struct atoms *atoms)
{
    token_expect(file, token_open);

    vm_reset(vm);
    ip_t ret = vm_exec(vm, mod);

    bool ok = true;
    while (!file_eof(file)) {
        struct token token = token_next(file);
        if (token.type == token_close) break;

        switch (token.type) {

        case token_reg: {
            reg_t reg = token.value.w;
            token_expect(file, token_assign);
            ok = ok && check_u64("reg", vm->regs[reg], token_word(file, atoms));
            break;
        }

        case token_stack: {
            size_t sp = token.value.w;
            token_expect(file, token_assign);
            ok = ok && check_u64("stack", vm->stack[sp], token_word(file, atoms));
            break;
        }

        case token_symbol: {
            uint64_t key = symbol_hash(&token.value.s);
            token_expect(file, token_assign);

            word_t exp = token_word(file, atoms);
            if (key == field("S")) ok = ok && check_u64("S", vm->specs.stack, exp);
            if (key == field("C")) ok = ok && check_u64("C", vm->specs.speed, exp);
            if (key == field("flags")) ok = ok && check_u64("flags", vm->flags, exp);
            if (key == field("io")) ok = ok && check_u64("io", vm->io, exp);
            if (key == field("ior")) ok = ok && check_u64("ior", vm->ior, exp);
            if (key == field("sp")) ok = ok && check_u64("sp", vm->sp, exp);
            if (key == field("ret")) ok = ok && check_u64("ret", ret, exp);

            break;
        }

        default: { assert(false); }
        }
    }

    if (!ok) {
        char buffer[s_page_len] = {0};
        dbg("\n<src>\n%s\n\n", mod->src);

        mod_dump(mod, buffer, sizeof(buffer));
        dbg("<bytecode:%x:%u>\n%s\n", (unsigned) mod->id, mod->len, buffer);

        vm_dbg(vm, buffer, sizeof(buffer));
        dbg("<ret>\n%sret:   %x\n\n", buffer, ret);
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
        token_expect(&file, token_open);
        print_title(&file);

        struct vm *vm = read_vm(&file);
        const struct mod *mod = read_mod(&file, mods, atoms);
        if (!mod) { ok = false; continue; }

        ok = ok && check_mod(&file, vm, mod, atoms);
        token_expect(&file, token_close);
    }

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

        ok = ok && check_file(file);
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
