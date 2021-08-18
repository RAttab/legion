/* db.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
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

static size_t db_len(enum item type)
{
    switch (type) {
    case ITEM_DB_1: { return db_len_s; }
    case ITEM_DB_2: { return db_len_m; }
    case ITEM_DB_3: { return db_len_l; }
    default: { assert(false); }
    }
}

static void db_init(void *state, id_t id, struct chunk *chunk)
{
    struct db *db = state;
    (void) chunk;

    db->id = id;
    db->len = db_len(id_item(id));
}

static void db_make(
        void *state, id_t id, struct chunk *chunk, const word_t *data, size_t len)
{
    struct db *db = state;
    db_init(db, id, chunk);

    if (len < 1) return;

    len = legion_min(len, db_len(id_item(id)));
    for (size_t i = 0; i < len; ++i)
        db->data[i] = data[i];
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void db_io_status(struct db *db, struct chunk *chunk, id_t src)
{
    word_t value = db->len;
    chunk_io(chunk, IO_STATE, db->id, src, 1, &value);
}

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

    switch (io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, db->id, src, 0, NULL); return; }
    case IO_STATUS: { db_io_status(db, chunk, src); return; }
    case IO_GET: { db_io_get(db, chunk, src, len, args); return; }
    case IO_SET: { db_io_set(db, len, args); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *db_config(enum item item)
{
    static const word_t io_list[] = { IO_PING, IO_STATUS, IO_GET, IO_SET };

    switch(item) {

    case ITEM_DB_1: {
        static const struct active_config config = {
            .size = sizeof(struct db) + db_len_s * sizeof(word_t),
            .init = db_init,
            .make = db_make,
            .step = NULL,
            .io = db_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_DB_2: {
        static const struct active_config config = {
            .size = sizeof(struct db) + db_len_m * sizeof(word_t),
            .init = db_init,
            .make = db_make,
            .step = NULL,
            .io = db_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_DB_3: {
        static const struct active_config config = {
            .size = sizeof(struct db) + db_len_l * sizeof(word_t),
            .init = db_init,
            .make = db_make,
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
