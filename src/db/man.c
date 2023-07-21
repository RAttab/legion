/* man.c
   Rémi Attab (remi.attab@gmail.com), 22 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/man.h"

// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct legion_packed man_db_header
{
    uint8_t magic;
    uint32_t path_len;
    uint32_t data_len;
};

bool man_db_next(struct man_db_it *it)
{
    extern const uint8_t db_man_list[];
    extern const uint8_t db_man_list_end[];

    if (!it->end) {
        *it = (struct man_db_it) {
            .it = db_man_list,
            .end = db_man_list_end
        };
    }
    if (it->it >= it->end) return false;

    struct man_db_header header = *((const struct man_db_header *) it->it);
    assert(header.magic == 0xFF);
    it->it += sizeof(header);

    it->path = it->it;
    it->path_len = header.path_len;
    it->it += header.path_len;

    it->data = it->it;
    it->data_len = header.data_len;
    it->it += header.data_len;

    assert(it->it <= it->end);
    return true;
}
