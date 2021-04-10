/* items.c
   Rémi Attab (remi.attab@gmail.com), 05 Apr 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db.h"

#include "render/core.h"
#include "utils/htable.h"
#include "utils/str.h"
#include "utils/log.h"

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


// -----------------------------------------------------------------------------
// db_items
// -----------------------------------------------------------------------------

struct
{
    struct htable from_schema;
} items;

item_t db_items_from_schema(const schema_t schema)
{
    struct htable_ret ret = htable_get(&items.from_schema, schema_hash(schema));
    return ret.ok ? ret.value : ITEM_NIL;
}


static char *items_hex(char *it, const char *end, uint8_t *ret)
{
    uint8_t val = 0;

    assert(it < end);
    val = str_charhex(*it);
    assert(val != 0xFF);
    *ret = val << 4;
    it++;

    assert(it < end);
    val = str_charhex(*it);
    assert(val != 0xFF);
    *ret |= val;
    it++;

    return it;
}

static char *items_expect(char *it, const char *end, char exp)
{
    assert(it < end);
    assert(*it == exp);
    return it + 1;
}

void db_items_load()
{
    assert(!items.from_schema.len);

    char path[PATH_MAX] = {0};
    core_path_res("db/items.db", path, sizeof(path));

    int fd = open(path, O_RDONLY);
    if (fd < 0) fail_errno("file not found: %s", path);

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

    size_t len = stat.st_size;
    char *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

    char *it = base;
    const char *end = it + len;
    htable_reserve(&items.from_schema, (end - it) / 84);

    while (it < end) {
        item_t item = 0;
        it = items_hex(it, end, &item);
        it = items_expect(it, end, ':');
        it = items_expect(it, end, '|');

        schema_t schema = {0};
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                for (size_t k = 0; k < 3; ++k) {
                    item_t bit = 0;
                    it = items_hex(it, end, &bit);

                    assert(bit < ITEM_MAX);
                    *schema_idx(schema, k, j, i) = bit;
                    it++;
                }
            }
        }
        it = items_expect(it, end, '\n');

        uint64_t hash = schema_hash(schema);
        struct htable_ret ret = htable_put(&items.from_schema, hash, item);
        assert(ret.ok);
    }

    munmap(base, len);
    close(fd);
}
