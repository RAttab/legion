/* db.c
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "game/save.h"
#include "utils/htable.h"
#include "utils/bits.h"
#include "utils/log.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct mod *mod_alloc(
        const char *src, size_t src_len,
        const uint8_t *code, size_t code_len,
        const struct mod_pub *pub, size_t pub_len,
        const struct mod_err *errs, size_t errs_len,
        const struct mod_index *index, size_t index_len)
{
    size_t head_bytes = sizeof(struct mod);
    size_t src_bytes = src_len + 1;
    size_t code_bytes = code_len * sizeof(*code);
    size_t pub_bytes = pub_len * sizeof(*pub);
    size_t errs_bytes = errs_len * sizeof(*errs);
    size_t index_bytes = index_len * sizeof(*index);
    size_t total_bytes =
        head_bytes + code_bytes + src_bytes + pub_bytes + errs_bytes + index_bytes;

    struct mod *mod = alloc_cache(align_cache(total_bytes));

    memcpy(&mod->code, code, code_bytes);
    mod->len = code_len;

    mod->src = ((void *) mod->code) + code_bytes;
    memcpy(mod->src, src, src_len);
    mod->src_len = src_bytes;
    mod->src[src_len] = 0;

    mod->pub = ((void *) mod->src) + src_bytes;
    memcpy(mod->pub, pub, pub_bytes);
    mod->pub_len = pub_len;

    mod->errs = ((void *) mod->pub) + pub_bytes;
    memcpy(mod->errs, errs, errs_bytes);
    mod->errs_len = errs_len;

    mod->index = ((void *) mod->errs) + errs_bytes;
    memcpy(mod->index, index, index_bytes);
    mod->index_len = index_len;

    return mod;
}

struct mod *mod_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_mod)) return NULL;

    mod_t id = save_read_type(save, typeof(id));
    uint32_t code_len = save_read_type(save, typeof(code_len));
    uint32_t src_len = save_read_type(save, typeof(src_len));
    uint32_t pub_len = save_read_type(save, typeof(pub_len));
    uint32_t index_len = save_read_type(save, typeof(index_len));

    size_t total_bytes = sizeof(struct mod) + code_len + src_len + pub_len + index_len;
    struct mod *mod = alloc_cache(align_cache(total_bytes));
    mod->id = id;

    void *it = mod + 1;

    mod->len = code_len;
    save_read(save, it, code_len);
    it += code_len;

    mod->src_len = src_len;
    mod->src = it;
    save_read(save, it, src_len);
    it += src_len;

    mod->pub_len = pub_len;
    mod->pub = it;
    save_read(save, it, pub_len);
    it += pub_len;

    mod->errs_len = 0;
    mod->errs = it;

    mod->index_len = index_len;
    mod->index = it;
    save_read(save, it, index_len);
    it += index_len;

    assert(it == ((void *) mod) + total_bytes);

    if (!save_read_magic(save, save_magic_mod)) { free(mod); return NULL; }
    return mod;
}

void mod_save(const struct mod *mod, struct save *save)
{
    assert(!mod->errs_len);

    save_write_magic(save, save_magic_mod);
    save_write_value(save, mod->id);
    save_write_value(save, mod->len);
    save_write_value(save, mod->src_len);
    save_write_value(save, mod->pub_len);
    save_write_value(save, mod->index_len);
    save_write(save, mod->code, mod->len);
    save_write(save, mod->src, mod->src_len);
    save_write(save, mod->pub, mod->pub_len);
    save_write(save, mod->index, mod->index_len);
    save_write_magic(save, save_magic_mod);
}

struct mod *mod_nil(mod_t id)
{
    struct mod *mod = alloc_cache(sizeof(*mod));
    void *end = mod+1;

    *mod = (struct mod) {
        .id = id,
        .src = end,
        .pub = end,
        .errs = end,
        .index = end,
    };

    return mod;
}

ip_t mod_pub(const struct mod *mod, uint64_t key)
{
    for (size_t i = 0; i < mod->pub_len; ++i) {
        if (mod->pub[i].key == key) return mod->pub[i].ip;
    }

    return 0;
}

struct mod_index mod_index(const struct mod *mod, ip_t ip)
{
    assert(!ip_is_mod(ip));
    assert(ip < mod->len);

    for (size_t i = 0; i < mod->index_len; ++i) {
        if (ip < mod->index[i+1].ip) return mod->index[i];
    }
    assert(false);
}

ip_t mod_byte(const struct mod *mod, size_t row, size_t col)
{
    for (size_t i = 0; i < mod->index_len; ++i) {
        struct mod_index *next = &mod->index[i+1];
        if (row < next->row && col < next->col)
            return mod->index[i].ip;
    }
    assert(false);
}

size_t mod_dump(const struct mod *mod, char *dst, size_t len)
{
    size_t orig = len;

    const uint8_t *in = mod->code;
    const uint8_t *end = mod->code + mod->len;

    void dump_op(const char *op)
    {
        size_t n = snprintf(dst, len, "[%02lx] %s ", in - mod->code, op);
        dst += n; len -= n; in += sizeof(enum op_code);
    }

    void dump_nil(void)
    {
        size_t n = snprintf(dst, len, "\n");
        dst += n; len -= n;
    }

    void dump_lit(void)
    {
        size_t n = snprintf(dst, len, "%016lx\n", *((word_t *) in));
        dst += n; len -= n; in += sizeof(word_t);
    }

    void dump_reg(void)
    {
        size_t n = snprintf(dst, len, "$%u\n", *((reg_t *) in));
        dst += n; len -= n; in += sizeof(reg_t);
    }

    void dump_len(void)
    {
        size_t n = snprintf(dst, len, "%x\n", *((uint8_t *) in));
        dst += n; len -= n; in += sizeof(uint8_t);
    }

    void dump_off(void)
    {
        size_t n = snprintf(dst, len, "@%x\n", *((ip_t *) in));
        dst += n; len -= n; in += sizeof(ip_t);
    }

    void dump_mod(void)
    {
        size_t n = snprintf(dst, len, "@%x\n", *((ip_t *) in));
        dst += n; len -= n; in += sizeof(ip_t);
    }

    while (in < end) {
        switch (*in)
        {
#define op_fn(op, arg) case OP_ ## op: { dump_op(#op); dump_ ## arg(); break; }
    #include "vm/op_xmacro.h"

        default: { assert(false); }
        }
    }

    return orig - len;
}

size_t mod_hexdump(const struct mod *mod, char *dst, size_t len)
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

struct mods
{
    mod_id_t id;
    struct htable by_id;
    struct htable by_mod;
};

struct mod_entry
{
    mod_id_t id;
    mod_ver_t ver;

    struct symbol str;
    const struct mod *mod;
};

struct mods *mods_new(void)
{
    return calloc(1, sizeof(struct mods));
}

void mods_free(struct mods *mods)
{
    struct htable_bucket *it = NULL;

    for (it = htable_next(&mods->by_id, NULL); it; it = htable_next(&mods->by_id, it))
        free((struct mod_entry *) it->value);
    htable_reset(&mods->by_id);

    for (htable_next(&mods->by_mod, NULL); it; it = htable_next(&mods->by_mod, it))
        free((struct mod *) it->value);
    htable_reset(&mods->by_mod);

    free(mods);
}

struct mods *mods_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_mods)) return NULL;

    struct mods *mods = mods_new();
    save_read_into(save, &mods->id);

    size_t mod_len = save_read_type(save, typeof(mods->by_mod.len));
    htable_reserve(&mods->by_mod, mod_len);
    for (size_t i = 0; i < mod_len; ++i) {
        struct mod *mod = mod_load(save);
        if (!mod) goto fail;

        struct htable_ret ret = htable_put(&mods->by_mod, mod->id, (uintptr_t) mod);
        assert(ret.ok);
    }

    size_t id_len = save_read_type(save, typeof(mods->by_id.len));
    htable_reserve(&mods->by_id, id_len);
    for (size_t i = 0; i < id_len; ++i) {
        struct mod_entry *entry = calloc(1, sizeof(*entry));
        save_read_into(save, &entry->id);
        save_read_into(save, &entry->ver);
        if (!save_read_symbol(save, &entry->str)) goto fail;

        struct htable_ret ret = htable_get(&mods->by_mod, make_mod(entry->id, entry->ver));
        if (!ret.ok) goto fail;
        entry->mod = (struct mod *) ret.value;

        ret = htable_put(&mods->by_id, entry->id, (uintptr_t) entry);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_mods)) goto fail;
    return mods;

  fail:
    mods_free(mods);
    return NULL;
}

void mods_save(const struct mods *mods, struct save *save)
{
    save_write_magic(save, save_magic_mods);
    save_write_value(save, mods->id);

    struct htable_bucket *it = NULL;

    save_write_value(save, mods->by_mod.len);
    for (it = htable_next(&mods->by_mod, NULL); it; it = htable_next(&mods->by_mod, it))
        mod_save((const struct mod *) it->value, save);

    save_write_value(save, mods->by_id.len);
    for (it = htable_next(&mods->by_id, NULL); it; it = htable_next(&mods->by_id, it)) {
        const struct mod_entry *entry = (const void *) it->value;
        save_write_value(save, entry->id);
        save_write_value(save, entry->ver);
        save_write_symbol(save, &entry->str);
    }

    save_write_magic(save, save_magic_mods);
}


mod_t mods_register(struct mods *mods, const struct symbol *name)
{
    struct mod_entry *entry = calloc(1, sizeof(*entry));
    entry->id = ++mods->id;
    entry->str = *name;

    mod_t mod = make_mod(entry->id, ++entry->ver);
    entry->mod = mod_nil(mod);

    struct htable_ret ret = {0};

    ret = htable_put(&mods->by_id, entry->id, (uintptr_t) entry);
    assert(ret.ok);

    ret = htable_put(&mods->by_mod, mod, (uintptr_t) entry->mod);
    assert(ret.ok);

    return mod;
}

bool mods_name(struct mods *mods, mod_id_t id, struct symbol *dst)
{
    if (!id) return false;

    struct htable_ret ret = htable_get(&mods->by_id, id);
    if (!ret.ok) return false;

    struct mod_entry *entry = (void *) ret.value;
    *dst = entry->str;
    return true;
}

mod_t mods_set(struct mods *mods, mod_id_t id, const struct mod *mod)
{
    assert(id);
    assert(!mod->errs_len);

    struct htable_ret ret = htable_get(&mods->by_id, id);
    if (!ret.ok) return 0;

    struct mod_entry *entry = (void *) ret.value;
    entry->mod = mod;
    entry->ver++;

    // a bit dirty but this is the only time where we actually want to change
    // something in mod.
    ((struct mod *) mod)->id = make_mod(id, entry->ver);
    ret = htable_put(&mods->by_mod, mod->id, (uintptr_t) mod);
    assert(ret.ok);

    return mod->id;
}

const struct mod *mods_latest(struct mods *mods, mod_id_t id)
{
    if (!id) return NULL;

    struct htable_ret ret = htable_get(&mods->by_id, id);
    return ret.ok ? ((struct mod_entry *) ret.value)->mod : NULL;
}

const struct mod *mods_get(struct mods *mods, mod_t id)
{
    if (!id) return NULL;
    if (!mod_ver(id)) return mods_latest(mods, mod_id(id));

    struct htable_ret ret = htable_get(&mods->by_mod, id);
    return ret.ok ? (struct mod *) ret.value : NULL;
}


mod_id_t mods_find(struct mods *mods, const struct symbol *name)
{
    struct htable_bucket *it = htable_next(&mods->by_id, NULL);
    for (; it; it = htable_next(&mods->by_id, it)) {
        struct mod_entry *entry = (void *) it->value;
        if (symbol_eq(name, &entry->str)) return entry->id;
    }
    return 0;
}

struct mods_list *mods_list(struct mods *mods)
{
    struct mods_list *ret = calloc(1,
            sizeof(*ret) + mods->by_id.len * sizeof(ret->items[0]));
    ret->len = mods->by_id.len;

    struct htable_bucket *it = htable_next(&mods->by_id, NULL);
    for (size_t i = 0; it; it = htable_next(&mods->by_id, it), i++) {
        struct mod_entry *entry = (void *) it->value;
        ret->items[i].id = entry->id;
        memcpy(&ret->items[i].str, &entry->str, sizeof(entry->str));
    }

    int mods_item_cmp(const void *lhs_, const void *rhs_)
    {
        const struct mods_item *lhs = lhs_;
        const struct mods_item *rhs = rhs_;
        return symbol_cmp(&lhs->str, &rhs->str);
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

static void mods_load_path(struct mods *mods, struct atoms *atoms, const char *path)
{
    struct mod *mod = NULL;
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0) fail_errno("file not found: %s", path);

        struct stat stat = {0};
        if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

        size_t len = stat.st_size;
        char *base = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

        mod = mod_compile(strnlen(base, len), base, mods, atoms);
        if (mod->errs_len) {
            for (size_t i = 0; i < mod->errs_len; ++i) {
                struct mod_err *err = &mod->errs[i];
                dbg("%s:%u:%u: %s", path, err->row, err->col, err->str);
            }
        }

        munmap(base, len);
        close(fd);
    }

    size_t dot = 0, slash = 0;
    for (size_t i = 0; path[i]; ++i) {
        if (path[i] == '/') slash = i;
        if (path[i] == '.') dot = i;
    }
    assert(slash < dot);

    struct symbol name = make_symbol_len(dot - slash - 1, path + slash + 1);

    mod_id_t id = mod_id(mods_register(mods, &name));
    mods_set(mods, id, mod);
}

void mods_populate(struct mods *mods, struct atoms *atoms)
{
    const char *path = "res/mods";

    DIR *dir = opendir(path);
    if (!dir) fail_errno("can't open dir: %s", path);

    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        char file[PATH_MAX];
        snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);

        mods_load_path(mods, atoms, file);
    }
    closedir(dir);
}
