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

struct legion_packed db
{
    id_t id;
    uint8_t len;
    legion_pad(3);
    word_t data[];
};

static_assert(sizeof(struct db) == 8);

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
    case ITEM_DB_S: { db->len = db_len_s; break; }
    case ITEM_DB_M: { db->len = db_len_m; break; }
    case ITEM_DB_L: { db->len = db_len_l; break; }
    default: { assert(false); }
    }
}

// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void db_cmd_get(
        struct db *db, struct chunk *chunk,
        id_t src, size_t len, const word_t *args)
{
    word_t val = 0;
    if (len >= 1) {
        word_t index = args[0];
        if (index >= 0 && index < db->len) val = db->data[index];
    }
    chunk_cmd(chunk, IO_VAL, db->id, src, 1, &val);
}

static void db_cmd_set(struct db *db, size_t len, const word_t *args)
{
    if (len < 2) return;
    word_t index = args[0];
    if (index < db->len) db->data[index] = args[1];
}

static void db_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct db *db = state;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, db->id, src, 0, NULL); return; }
    if (cmd == IO_GET) { db_cmd_get(db, chunk, src, len, args); return; }
    if (cmd == IO_SET) { db_cmd_set(db, len, args); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *db_config(item_t item)
{
    switch(item) {

    case ITEM_DB_S: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_s * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .cmd = db_cmd,
        };
        return &config;
    }

    case ITEM_DB_M: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_m * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .cmd = db_cmd,
        };
        return &config;
    }

    case ITEM_DB_L: {
        static const struct item_config config = {
            .size = sizeof(struct db) + db_len_l * sizeof(word_t),
            .init = db_init,
            .step = NULL,
            .cmd = db_cmd,
        };
        return &config;
    }

    default: { assert(false); }
    }
}
