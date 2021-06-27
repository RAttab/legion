/* db.c
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "utils/htable.h"
#include "utils/bits.h"
#include "utils/log.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct mod *mod_alloc(
        const struct text *src,
        const uint8_t *code, size_t code_len,
        const struct mod_err *errs, size_t errs_len,
        const struct mod_index *index, size_t index_len)
{
    size_t head_bytes = sizeof(struct mod);
    size_t src_bytes = src->len * (text_line_cap + 1);
    size_t code_bytes = code_len * sizeof(*code);
    size_t errs_bytes = errs_len * sizeof(*errs);
    size_t index_bytes = (index_len + 1) * sizeof(*index);
    size_t total_bytes = align_cache(
            head_bytes + code_bytes + src_bytes + errs_bytes + index_bytes);

    struct mod *mod = alloc_cache(total_bytes);
    ref_init(mod);

    memcpy(&mod->code, code, code_bytes);
    mod->len = code_len;

    mod->src = ((void *) mod->code) + code_bytes;
    text_pack(src, mod->src, src_bytes);
    mod->src_len = src_bytes;

    mod->errs = ((void *) mod->src) + src_bytes;
    memcpy(mod->errs, errs, errs_bytes);
    mod->errs_len = errs_len;

    mod->index = ((void *) mod->errs) + errs_bytes;
    memcpy(mod->index, index, index_bytes);
    mod->index[index_len] = (struct mod_index) { .line = src->len, .byte = code_len };
    mod->index_len = index_len;

    return mod;
}

struct mod *mod_nil(mod_t id)
{
    struct mod *mod = alloc_cache(sizeof(*mod));
    void *end = mod+1;

    *mod = (struct mod) {
        .id = id,
        .src = end,
        .errs = end,
        .index = end,
    };
    ref_init(mod);

    return mod;
}

size_t mod_line(struct mod *mod, ip_t ip)
{
    addr_t byte = ip_addr(ip);
    assert(byte < mod->len);

    for (size_t i = 0; i < mod->index_len; ++i) {
        if (byte < mod->index[i+1].byte)
            return mod->index[i].line;
    }
    assert(false);
}

addr_t mod_byte(struct mod *mod, size_t line)
{
    assert(line < mod->src_len);

    for (size_t i = 0; i < mod->index_len; ++i) {
        if (line < mod->index[i+1].line)
            return mod->index[i].byte;
    }
    assert(false);
}

size_t mod_dump(struct mod *mod, char *dst, size_t len)
{
    size_t orig = len;

    for (size_t i = 0; i < mod->index_len; ++i) {
        struct mod_index *curr = &mod->index[i];
        struct mod_index *next = curr + 1;

        size_t n = snprintf(dst, len, "[%04u:%02x] %02x ",
                curr->line, curr->byte, mod->code[curr->byte]);
        dst += n; len -= n;

        void *arg = mod->code + curr->byte + 1;

        size_t arg_len = next->byte - (curr->byte + 1);
        switch (arg_len) {
        case 0: { n = snprintf(dst, len, "\n"); break; }
        case 1: { n = snprintf(dst, len, "%02x\n", (unsigned) *((uint8_t *) arg)); break; }
        case 2: { n = snprintf(dst, len, "%04x\n", (unsigned) *((uint16_t *) arg)); break; }
        case 4: { n = snprintf(dst, len, "%08x\n", *((uint32_t *) arg)); break; }
        case 8: { n = snprintf(dst, len, "%016lx\n", *((uint64_t *) arg)); break; }
        default: { assert(false); }
        }

        dst += n; len -= n;
    }

    return orig - len;
}

size_t mod_hexdump(struct mod *mod, char *dst, size_t len)
{
    size_t orig = len;

    for (size_t i = 0; i < mod->len; ++i) {
        if (i % 16 == 0) {
            if (i) { snprintf(dst, len, "\n"); dst++; len--; }
            size_t n = snprintf(dst, len, "[%02x] ", (unsigned) i);
            dst += n; len -= n;
        }

        size_t n = snprintf(dst, len, "%02x ", mod->code[i]);
        dst += n; len -= n;
    }

    size_t n = snprintf(dst, len, "\n");
    dst += n; len -= n;

    return orig - len;
}


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct mods_entry
{
    mod_t id;
    atom_t str;
    struct mod *mod;
};

static struct
{
    mod_t ids;
    struct htable index;
} mods;


void mods_free()
{
    struct htable_bucket *it = htable_next(&mods.index, NULL);
    for (; it; it = htable_next(&mods.index, it)) {
        struct mods_entry *entry = (void *) it->value;
        mod_discard(entry->mod);
        free(entry);
    }
    htable_reset(&mods.index);
}

mod_t mods_register(const atom_t *name)
{
    struct mods_entry *entry = calloc(1, sizeof(*entry));
    memcpy(entry->str, name, vm_atom_cap);

    entry->id = ++mods.ids;
    assert(entry->id);

    entry->mod = mod_nil(entry->id);

    struct htable_ret ret = htable_put(&mods.index, entry->id, (uint64_t) entry);
    assert(ret.ok);

    return entry->id;
}

bool mods_name(mod_t id, atom_t *dst)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    memcpy(dst, entry->str, vm_atom_cap);
    return true;
}

bool mods_del(mod_t id)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    if (entry->mod) mod_discard(entry->mod);

    ret = htable_del(&mods.index, id);
    assert(ret.ok);

    return true;
}

bool mods_store(mod_t id, struct mod *mod)
{
    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return false;

    struct mods_entry *entry = (void *) ret.value;
    if (entry->mod) mod_discard(entry->mod);
    entry->mod = mod_share(mod);
    mod->id = id;

    return true;
}

struct mod *mods_load(mod_t id)
{
    if (!id) return NULL;

    struct htable_ret ret = htable_get(&mods.index, id);
    if (!ret.ok) return NULL;

    struct mods_entry *entry = (void *) ret.value;
    return mod_share(entry->mod);
}

mod_t mods_find(const atom_t *name)
{
    struct htable_bucket *it = htable_next(&mods.index, NULL);
    for (; it; it = htable_next(&mods.index, it)) {
        struct mods_entry *entry = (void *) it->value;
        if (vm_atoms_eq(name, &entry->str)) return entry->id;
    }
    return 0;
}

static int mods_item_cmp(const void *lhs_, const void *rhs_)
{
    const struct mods_item *lhs = lhs_;
    const struct mods_item *rhs = rhs_;
    return vm_atoms_cmp(&lhs->str, &rhs->str);
}

struct mods *mods_list(void)
{
    struct mods *ret = calloc(1,
            sizeof(*ret) + mods.index.len * sizeof(ret->items[0]));
    ret->len = mods.index.len;

    struct htable_bucket *it = htable_next(&mods.index, NULL);
    for (size_t i = 0; it; it = htable_next(&mods.index, it), i++) {
        struct mods_entry *entry = (void *) it->value;
        ret->items[i].id = entry->id;
        memcpy(ret->items[i].str, entry->str, vm_atom_cap);
    }

    qsort(ret->items, ret->len, sizeof(ret->items[0]), mods_item_cmp);
    return ret;
}


// -----------------------------------------------------------------------------
// persist
// -----------------------------------------------------------------------------

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

static void mods_load_path(const char *path)
{
    struct mod *mod= NULL;
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0) fail_errno("file not found: %s", path);

        struct stat stat = {0};
        if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

        size_t len = stat.st_size;
        char *base = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

        struct text src = {0};
        text_from_str(&src, base, strnlen(base, len));

        mod = mod_compile(&src);
        if (mod->errs_len) {
            for (size_t i = 0; i < mod->errs_len; ++i) {
                struct mod_err *err = &mod->errs[i];
                dbg("%s:%zu: %s", path, err->line, err->str);
            }
        }

        text_clear(&src);
        munmap(base, len);
        close(fd);
    }

    size_t dot = 0, slash = 0;
    for (size_t i = 0; path[i]; ++i) {
        if (path[i] == '/') slash = i;
        if (path[i] == '.') dot = i;
    }
    assert(slash < dot);

    atom_t name = {0};
    memcpy(name, path + slash + 1, dot - slash - 1);

    mod_t id = mods_register(&name);
    assert(mods_store(id, mod));
    dbg("mod: %s -> %x", &name[0], id);

    mod_discard(mod);
}

void mods_preload(void)
{
    const char *path = "res/mods";

    DIR *dir = opendir(path);
    if (!dir) fail_errno("can't open dir: %s", path);

    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        char file[PATH_MAX];
        snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);

        mods_load_path(file);
    }
    closedir(dir);
}
