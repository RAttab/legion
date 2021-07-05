/* db.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

enum
{
    db_len_s = 7 + 8 * 0,
    db_len_m = 7 + 8 * 1,
    db_len_l = 7 + 8 * 2,
};

static void db_init(void *state, id_t id, struct chunk *chunk)
{
    struct db *db = state;
    (void) chunk;

    db->id = id;
    switch (id_item(id)) {
    case ITEM_DB_I:   { db->len = db_len_s; break; }
    case ITEM_DB_II:  { db->len = db_len_m; break; }
    case ITEM_DB_III: { db->len = db_len_l; break; }
    default: { assert(false); }
    }
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void db_io_get(
        struct db *db, struct chunk *chunk,
        id_t src, size_t len, const word_t *args)
{
    word_t val = 0;
    if (len >= 1) {
        word_t index = args[0];
        if (index >= 0 && index < db->len) val = db->data[index];
    }
    chunk_io(chunk, IO_VAL, db->id, src, 1, &val);
}

static void db_io_set(struct db *db, size_t len, const word_t *args)
{
    if (len < 2) return;
    word_t index = args[0];
    if (index < db->len) db->data[index] = args[1];
}

static void db_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct db *db = state;

    if (io == IO_PING) { chunk_io(chunk, IO_PONG, db->id, src, 0, NULL); return; }
    if (io == IO_GET) { db_io_get(db, chunk, src, len, args); return; }
    if (io == IO_SET) { db_io_set(db, len, args); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *db_config(item_t item)
{
    static const word_t io_list[] = { IO_PING, IO_GET, IO_SET };

    switch(item) {

    case ITEM_DB_I: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_s * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .io = db_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_DB_II: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_m * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .io = db_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_DB_III: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_l * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .io = db_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    default: { assert(false); }
    }
}
