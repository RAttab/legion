/* img.c
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct legion_packed db_man_header
{
    uint8_t magic;
    uint32_t path_len;
    uint32_t data_len;
};

bool db_man_next(struct db_man_it *it)
{
    extern const uint8_t db_man_list[];
    extern const uint8_t db_man_list_end[];

    if (!it->end) {
        *it = (struct db_man_it) {
            .it = db_man_list,
            .end = db_man_list_end
        };
    }
    if (it->it >= it->end) return false;

    struct db_man_header header = *((const struct db_man_header *) it->it);
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
