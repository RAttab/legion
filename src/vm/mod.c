/* db.c
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "game/sys.h"
#include "utils/save.h"
#include "utils/fs.h"
#include "utils/bits.h"
#include "utils/str.h"
#include "utils/htable.h"


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

void mod_free(const struct mod *mod)
{
    free((void *) mod);
}

struct mod *mod_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_mod)) return NULL;
    struct mod *mod = NULL;

    mod_id id = save_read_type(save, typeof(id));

    uint32_t code_len = save_read_type(save, typeof(code_len));
    size_t code_bytes = code_len * sizeof(*mod->code);

    uint32_t src_len = save_read_type(save, typeof(src_len));
    size_t src_bytes = src_len * sizeof(*mod->src);

    uint32_t pub_len = save_read_type(save, typeof(pub_len));
    size_t pub_bytes = pub_len * sizeof(*mod->pub);

    uint32_t errs_len = save_read_type(save, typeof(errs_len));
    size_t errs_bytes = errs_len * sizeof(*mod->errs);

    uint32_t index_len = save_read_type(save, typeof(index_len));
    size_t index_bytes = index_len * sizeof(*mod->index);

    size_t total_bytes = sizeof(*mod)
        + code_bytes
        + src_bytes
        + pub_bytes
        + errs_bytes
        + index_bytes;
    mod = alloc_cache(align_cache(total_bytes));
    mod->id = id;

    void *it = mod + 1;

    mod->len = code_len;
    save_read(save, it, code_bytes);
    it += code_bytes;

    mod->src_len = src_len;
    mod->src = it;
    save_read(save, it, src_bytes);
    it += src_bytes;

    mod->pub_len = pub_len;
    mod->pub = it;
    save_read(save, it, pub_bytes);
    it += pub_bytes;

    mod->errs_len = errs_len;
    mod->errs = it;
    save_read(save, it, errs_bytes);
    it += errs_bytes;

    mod->index_len = index_len;
    mod->index = it;
    save_read(save, it, index_bytes);
    it += index_bytes;

    assert(it == ((void *) mod) + total_bytes);

    if (!save_read_magic(save, save_magic_mod)) { free(mod); return NULL; }
    return mod;
}

void mod_save(const struct mod *mod, struct save *save)
{
    save_write_magic(save, save_magic_mod);
    save_write_value(save, mod->id);
    save_write_value(save, mod->len);
    save_write_value(save, mod->src_len);
    save_write_value(save, mod->pub_len);
    save_write_value(save, mod->errs_len);
    save_write_value(save, mod->index_len);
    save_write(save, mod->code, mod->len * sizeof(*mod->code));
    save_write(save, mod->src, mod->src_len * sizeof(*mod->src));
    save_write(save, mod->pub, mod->pub_len * sizeof(*mod->pub));
    save_write(save, mod->errs, mod->errs_len * sizeof(*mod->errs));
    save_write(save, mod->index, mod->index_len * sizeof(*mod->index));
    save_write_magic(save, save_magic_mod);
}

struct mod *mod_nil(mod_id id)
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

vm_ip mod_pub(const struct mod *mod, uint64_t key)
{
    for (size_t i = 0; i < mod->pub_len; ++i) {
        if (mod->pub[i].key == key) return mod->pub[i].ip;
    }

    return MOD_PUB_UNKNOWN;
}

struct mod_index mod_index(const struct mod *mod, vm_ip ip)
{
    assert(ip < mod->len);

    for (size_t i = 0; i < mod->index_len; ++i) {
        if (ip < mod->index[i+1].ip) return mod->index[i];
    }

    return mod->index[mod->index_len-1];
}

vm_ip mod_byte(const struct mod *mod, size_t row, size_t col)
{
    if (!mod->index_len) return 0;

    const struct mod_index *it = mod->index;
    const struct mod_index *end = it + mod->index_len;

    for (; it < end; it++) {
        if (row > it->row) continue;
        if (row < it->row) return it->ip;
        if (col < it->col + it->len) return it->ip;
    }

    return mod->index[mod->index_len-1].ip;
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
        size_t n = snprintf(dst, len, "%016lx\n", *((vm_word *) in));
        dst += n; len -= n; in += sizeof(vm_word);
    }

    void dump_reg(void)
    {
        size_t n = snprintf(dst, len, "$%u\n", *((vm_reg *) in));
        dst += n; len -= n; in += sizeof(vm_reg);
    }

    void dump_len(void)
    {
        size_t n = snprintf(dst, len, "%x\n", *((uint8_t *) in));
        dst += n; len -= n; in += sizeof(uint8_t);
    }

    void dump_off(void)
    {
        size_t n = snprintf(dst, len, "@%x\n", *((vm_ip *) in));
        dst += n; len -= n; in += sizeof(vm_ip);
    }

    void dump_mod(void)
    {
        size_t n = snprintf(dst, len, "@%lx\n", *((vm_word *) in));
        dst += n; len -= n; in += sizeof(vm_word);
    }

    while (in < end) {
        switch (*in)
        {
#define op_fn(op, arg) case OP_ ## op: { dump_op(#op); dump_ ## arg(); break; }
    #include "vm/op_xmacro.h"

        default: {
            dbgf("mod.dump.err: %x", *in);
            assert(false);
        }
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
    mod_maj maj;
    struct htable by_maj;
    struct htable by_mod;
};

struct mod_entry
{
    mod_maj maj;
    mod_ver ver;
    user_id owner;

    struct symbol str;
    const struct mod *mod;
};

struct mods *mods_new(void)
{
    struct mods *mods = calloc(1, sizeof(struct mods));
    return mods;
}

void mods_free(struct mods *mods)
{
    const struct htable_bucket *it = NULL;

    for (it = htable_next(&mods->by_maj, NULL); it; it = htable_next(&mods->by_maj, it))
        free((struct mod_entry *) it->value);
    htable_reset(&mods->by_maj);

    for (it = htable_next(&mods->by_mod, NULL); it; it = htable_next(&mods->by_mod, it))
        mod_free((struct mod *) it->value);
    htable_reset(&mods->by_mod);

    free(mods);
}

void mods_save(const struct mods *mods, struct save *save)
{
    save_write_magic(save, save_magic_mods);
    save_write_value(save, mods->maj);

    const struct htable_bucket *it = NULL;

    save_write_value(save, mods->by_mod.len);
    for (it = htable_next(&mods->by_mod, NULL); it; it = htable_next(&mods->by_mod, it))
        mod_save((const struct mod *) it->value, save);

    save_write_value(save, mods->by_maj.len);
    for (it = htable_next(&mods->by_maj, NULL); it; it = htable_next(&mods->by_maj, it)) {
        const struct mod_entry *entry = (const void *) it->value;
        save_write_value(save, entry->maj);
        save_write_value(save, entry->ver);
        save_write_value(save, entry->owner);
        symbol_save(&entry->str, save);
    }

    save_write_magic(save, save_magic_mods);
}

struct mods *mods_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_mods)) return NULL;

    struct mods *mods = mods_new();
    save_read_into(save, &mods->maj);

    size_t mod_len = save_read_type(save, typeof(mods->by_mod.len));
    htable_reserve(&mods->by_mod, mod_len);
    for (size_t i = 0; i < mod_len; ++i) {
        struct mod *mod = mod_load(save);
        if (!mod) goto fail;

        struct htable_ret ret = htable_put(&mods->by_mod, mod->id, (uintptr_t) mod);
        assert(ret.ok);
    }

    size_t id_len = save_read_type(save, typeof(mods->by_maj.len));
    htable_reserve(&mods->by_maj, id_len);
    for (size_t i = 0; i < id_len; ++i) {
        struct mod_entry *entry = calloc(1, sizeof(*entry));
        save_read_into(save, &entry->maj);
        save_read_into(save, &entry->ver);
        save_read_into(save, &entry->owner);
        if (!symbol_load(&entry->str, save)) goto fail;

        struct htable_ret ret = htable_get(&mods->by_mod, make_mod(entry->maj, entry->ver));
        if (!ret.ok) goto fail;
        entry->mod = (struct mod *) ret.value;

        ret = htable_put(&mods->by_maj, entry->maj, (uintptr_t) entry);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_mods)) goto fail;
    return mods;

  fail:
    mods_free(mods);
    return NULL;
}


mod_id mods_register(
        struct mods *mods, user_id owner, const struct symbol *name)
{
    if (mods_find(mods, name)) return 0;

    struct mod_entry *entry = calloc(1, sizeof(*entry));
    entry->maj = ++mods->maj;
    entry->str = *name;
    entry->owner = owner;

    mod_id mod = make_mod(entry->maj, ++entry->ver);
    entry->mod = mod_nil(mod);

    struct htable_ret ret = {0};

    ret = htable_put(&mods->by_maj, entry->maj, (uintptr_t) entry);
    assert(ret.ok);

    ret = htable_put(&mods->by_mod, mod, (uintptr_t) entry->mod);
    assert(ret.ok);

    return mod;
}

bool mods_name(struct mods *mods, mod_maj maj, struct symbol *dst)
{
    if (!maj) return false;

    struct htable_ret ret = htable_get(&mods->by_maj, maj);
    if (!ret.ok) return false;

    struct mod_entry *entry = (void *) ret.value;
    *dst = entry->str;
    return true;
}

user_id mods_owner(struct mods *mods, mod_maj maj)
{
    if (!maj) return -1;

    struct htable_ret ret = htable_get(&mods->by_maj, maj);
    if (!ret.ok) return -1;

    return ((struct mod_entry *) ret.value)->owner;
}

mod_id mods_set(struct mods *mods, mod_maj maj, const struct mod *mod)
{
    assert(maj);
    assert(!mod->errs_len);

    struct htable_ret ret = htable_get(&mods->by_maj, maj);
    if (!ret.ok) return 0;

    struct mod_entry *entry = (void *) ret.value;
    entry->mod = mod;
    entry->ver++;

    // a bit dirty but this is the only time where we actually want to change
    // something in mod.
    ((struct mod *) mod)->id = make_mod(maj, entry->ver);
    ret = htable_put(&mods->by_mod, mod->id, (uintptr_t) mod);
    assert(ret.ok);

    return mod->id;
}

const struct mod *mods_latest(struct mods *mods, mod_maj maj)
{
    if (!maj) return NULL;

    struct htable_ret ret = htable_get(&mods->by_maj, maj);
    return ret.ok ? ((struct mod_entry *) ret.value)->mod : NULL;
}

const struct mod *mods_get(struct mods *mods, mod_id id)
{
    if (!id) return NULL;
    if (!mod_version(id)) return mods_latest(mods, mod_major(id));

    struct htable_ret ret = htable_get(&mods->by_mod, id);
    return ret.ok ? (struct mod *) ret.value : NULL;
}

mod_maj mods_find(struct mods *mods, const struct symbol *name)
{
    const struct htable_bucket *it = htable_next(&mods->by_maj, NULL);
    for (; it; it = htable_next(&mods->by_maj, it)) {
        struct mod_entry *entry = (void *) it->value;
        if (symbol_eq(name, &entry->str)) return entry->maj;
    }
    return 0;
}

const struct mod *mods_parse(struct mods *mods, const char *it, size_t len)
{
    const char *end = it + len;

    if (*it != '(') return NULL;
    it++;

    struct symbol symbol = {0};
    ssize_t ret = symbol_parse(it, end - it, &symbol);
    if (ret == -1) return NULL;
    it += ret;

    mod_maj maj = mods_find(mods, &symbol);
    if (!maj) return NULL;

    it += str_skip_spaces(it, end - it);

    mod_ver ver = 0;
    if (*it != ')') {
        const char *start = it;
        while (it < end && str_is_number(*it)) it++;

        uint64_t value = 0;
        (void) str_atou(start, it - start, &value);
        if (value > UINT16_MAX) return NULL;

        ver = value;
    }

    return mods_get(mods, make_mod(maj, ver));
}

struct mods_list *mods_list(struct mods *mods, user_set filter)
{
    struct mods_list *ret = calloc(1,
            sizeof(*ret) + mods->by_maj.len * sizeof(ret->items[0]));
    ret->cap = mods->by_maj.len;

    const struct htable_bucket *it = htable_next(&mods->by_maj, NULL);
    for (; it; it = htable_next(&mods->by_maj, it)) {
        struct mod_entry *entry = (void *) it->value;
        if (!user_set_test(filter, entry->owner)) continue;

        struct mods_item *item = ret->items + ret->len;
        item->maj = entry->maj;
        item->ver = entry->ver;
        memcpy(&item->str, &entry->str, sizeof(entry->str));
        ret->len++;
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

struct mods_list *mods_list_reserve(size_t len)
{
    struct mods_list *list =
        calloc(1, sizeof(*list) + len * sizeof(list->items[0]));
    list->cap = len;
    return list;
}

void mods_list_save(struct mods *mods, struct save *save, user_set filter)
{
    struct mods_list *list = mods_list(mods, filter);
    save_write_magic(save, save_magic_mods);

    save_write_value(save, list->len);
    for (size_t i = 0; i < list->len; ++i) {
        const struct mods_item *it = list->items + i;
        save_write_value(save, it->maj);
        save_write_value(save, it->ver);
        symbol_save(&it->str, save);
    }

    save_write_magic(save, save_magic_mods);
    free(list);
}

bool mods_list_load_into(struct mods_list **ret, struct save *save)
{
    struct mods_list *list = *ret;
    if (!save_read_magic(save, save_magic_mods)) return false;

    size_t len = save_read_type(save, typeof(list->len));
    if (!list || len > list->cap) {
        list = realloc(list, list->cap * sizeof(list->items[0]));
        list->cap = len;
        *ret = list;
    }
    list->len = len;

    for (size_t i = 0; i < list->len; ++i) {
        struct mods_item *it = list->items + i;
        save_read_into(save, &it->maj);
        save_read_into(save, &it->ver);
        if (!symbol_load(&it->str, save)) return false;
    }

    return save_read_magic(save, save_magic_mods);
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static struct symbol mods_file_symbol(const char *path)
{
    size_t dot = 0, slash = 0;
    for (size_t i = 0; path[i]; ++i) {
        if (path[i] == '/') slash = i;
        if (path[i] == '.') dot = i;
    }
    assert(slash < dot);

    return make_symbol_len(path + slash + 1, dot - slash - 1);
}

static void mods_file_register(struct mods *mods, const char *path)
{
    if (!str_ends_with(path, ".lisp")) return;

    struct symbol name = mods_file_symbol(path);
    mod_maj maj = mods_register(mods, user_admin, &name);
    assert(maj);
}

static void mods_file_compile(
        struct mods *mods, struct atoms *atoms, const char *path)
{
    if (!str_ends_with(path, ".lisp")) return;

    struct symbol name = mods_file_symbol(path);
    mod_maj maj = mods_find(mods, &name);
    assert(maj);

    struct mod *mod = NULL;
    struct mfile file = mfile_open(path);

    mod = mod_compile(maj, file.ptr, file.len, mods, atoms);
    if (mod->errs_len) {
        for (size_t i = 0; i < mod->errs_len; ++i) {
            struct mod_err *err = &mod->errs[i];
            dbgf("%s:%u:%u: %s", path, err->row+1, err->col+1, err->str);
        }
    }

    mods_set(mods, maj, mod);
    mfile_close(&file);
}

// If we have cyclic module reference (it happens and it's useful) we need to
// register before we compile.
void mods_populate(struct mods *mods, struct atoms *atoms)
{
    char path[PATH_MAX] = {0};
    sys_path_mods(path, sizeof(path));

    {
        struct dir_it *it = dir_it(path);
        while (dir_it_next(it)) mods_file_register(mods, dir_it_path(it));
        dir_it_free(it);
    }

    {
        struct dir_it *it = dir_it(path);
        while (dir_it_next(it)) mods_file_compile(mods, atoms, dir_it_path(it));
        dir_it_free(it);
    }
}
