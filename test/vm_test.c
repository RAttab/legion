/* vm_test.c
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/vm.h"
#include "vm/mod.h"
#include "game/atoms.h"
#include "utils/text.h"
#include "utils/log.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

struct test
{
    atom_t title;
    const char *src;
    struct vm *in, *exp;
    ip_t exp_ret;
};

#define check_u64(title, field, _tval_, _texp_)                 \
    ({                                                          \
        uint64_t _val_ = (_tval_);                              \
        uint64_t _exp_ = (_texp_);                              \
        bool ok = _val_ == _exp_;                               \
        if (!ok) {                                              \
            fprintf(stderr, "%s.%s> val:%lx != exp:%lx\n",      \
                    title, field, _val_, _exp_);                \
        }                                                       \
        ok;                                                     \
    })

bool check(struct test *test)
{
    {
        char title[80];
        memset(title, '=', sizeof(title));
        title[sizeof(title) - 1] = 0;
        size_t len = snprintf(title, sizeof(title), "[ %s ]", test->title);
        title[len] = '=';
        fprintf(stderr, "%s\n", title);
    }

    assert(test->in);
    assert(test->exp);

    struct text src = {0};
    text_from_str(&src, test->src, strlen(test->src));
    struct mod *mod = mod_compile(&src);

    mod_t key = mods_register(&test->title);
    assert(mods_store(key, mod));
    test->in->ip = make_ip(key, 0);

    if (mod->errs_len) {
        for (size_t i = 0; i < mod->errs_len; ++i) {
            struct mod_err *err = &mod->errs[i];
            fprintf(stderr, "%zu: %s\n", err->line, err->str);
        }
        return false;
    }

    struct vm *vm = test->in;
    ip_t ret = vm_exec(vm, mod);


    bool ok = true;
    struct vm *exp = test->exp;
    const char *title = test->title;
    ok = check_u64(title, "flags", vm->flags, exp->flags) && ok;
    ok = check_u64(title, "io", vm->io, exp->io) && ok;
    ok = check_u64(title, "ior", vm->ior, exp->ior) && ok;
    ok = check_u64(title, "sp", vm->sp, exp->sp) && ok;
    ok = check_u64(title, "ret", ret, test->exp_ret) && ok;
    ok = (!exp->ip || check_u64(title, "ip", vm->ip, exp->ip)) && ok;
    ok = (!exp->tsc || check_u64(title, "tsc", vm->tsc, exp->tsc)) && ok;

    for (size_t i = 0; i < 4; ++i) {
        char reg[32]; sprintf(reg, "r%zu", i);
        ok = check_u64(title, reg, vm->regs[i], exp->regs[i]) && ok;
    }

    for (size_t i = 0; i < vm->sp; ++i) {
        char stack[32]; sprintf(stack, "s%zu", i);
        ok = check_u64(title, stack, vm->stack[i], exp->stack[i]) && ok;
    }

    if (!ok) {
        char buffer[1024] = {0};
        fprintf(stderr, "\n<src>\n%s\n\n", test->src);

        mod_dump(mod, buffer, sizeof(buffer));
        fprintf(stderr, "<bytecode:%x:%u>\n%s\n", (unsigned) mod->id, mod->len, buffer);

        vm_dbg(test->in, buffer, sizeof(buffer));
        fprintf(stderr, "<ret>\n%sret:   %x\n\n", buffer, ret);

        vm_dbg(test->exp, buffer, sizeof(buffer));
        fprintf(stderr, "<exp>\n%sret:   %x\n\n", buffer, test->exp_ret);
    }

    mod_discard(mod);
    text_clear(&src);
    free(test->exp);
    free(test->in);
    return ok;
}

char *read_field(char *ptr, struct test *test, struct vm **vm)
{
    const char *field = ptr;
    while (*ptr != ':') ptr++;
    *ptr = 0; ptr++;

    const char *str = ptr;
    while (*ptr != ',' && *ptr != '\n') ptr++;
    *ptr = 0; ptr++;

    word_t word = (str[0] == '0' && str[1] == 'x') ?
        (word_t) strtoul(str, NULL, 16) : strtol(str, NULL, 10);

    if (!strcmp(field, "S")) { *vm = vm_alloc(word, 4); goto end; }
    if (!strcmp(field, "C")) { (*vm)->specs.speed = 1 << word; goto end; }
    if (!strcmp(field, "flags")) { (*vm)->flags = word; goto end; }
    if (!strcmp(field, "tsc")) { (*vm)->tsc = word; goto end; }
    if (!strcmp(field, "ip")) { (*vm)->ip = word; goto end; }
    if (!strcmp(field, "sp")) { (*vm)->sp = word; goto end; }
    if (!strcmp(field, "ior")) { (*vm)->ior = word; goto end; }
    if (!strcmp(field, "io")) { (*vm)->io = word; goto end; }
    if (!strcmp(field, "ret")) { test->exp_ret = word; goto end; }
    if (*field == 'r') { (*vm)->regs[*(field + 1) - '1'] = word; goto end; }
    if (*field == 's') { (*vm)->stack[*(field + 1) - '0'] = word; goto end; }
    dbg("unknown field: %s -> %ld", field, word);
    assert(false);

  end:
    return ptr;
}

bool check_file(const char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd < 0) fail_errno("file not found: %s", file);

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", file);

    size_t len = stat.st_size;
    char *base = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) fail_errno("failed to mmap: %s", file);

    bool ok = true;
    char *ptr = base;
    char *end = ptr + len;

    while (ptr < end) {
        struct test test = {0};

        while (*ptr == '#')
            while (*ptr != '\n') { ptr++; }

        while (*ptr != '!') ptr++;
        const char *title = ptr + 1;
        while (*ptr != '\n') {ptr++; }
        memcpy(test.title, title, ptr - title);
        *ptr = 0; ptr++;

        const char prog[] = "&prog\n";
        assert(!strncmp(ptr, prog, sizeof(prog)-1));
        ptr += sizeof(prog)-1;
        test.src = ptr;

        while (*ptr != '&') { ptr++; }
        *(ptr - 1) = 0;

        while (*ptr != '\n' && ptr < end) {
            struct vm **vm = NULL;

            const char in[] = "&in>";
            const char out[] = "&out>";
            if (!strncmp(ptr, in, sizeof(in)-1)) { vm = &test.in; ptr += sizeof(in)-1; }
            else if (!strncmp(ptr, out, sizeof(out)-1)) { vm = &test.exp; ptr += sizeof(out)-1; }
            else assert(false);

            while (*ptr != '&' && *ptr != '\n' && ptr < end)
                ptr = read_field(ptr, &test, vm);
        }

        ok = check(&test) && ok;
    }

    munmap(base, len);
    close(fd);
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


int main(int argc, char **argv)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/test/vm", argc > 1 ? argv[1] : ".");

    vm_atoms_init();
    atoms_register();
    vm_compile_init();

    bool ok = check_dir(path);
    mods_free();

    return ok ? 0 : 1;
}
