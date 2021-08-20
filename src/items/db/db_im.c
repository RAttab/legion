/* db_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "items/io.h"


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

static size_t im_db_len(enum item type)
{
    switch (type) {
    case ITEM_DB_1: { return im_db_len_s; }
    case ITEM_DB_2: { return im_db_len_m; }
    case ITEM_DB_3: { return im_db_len_l; }
    default: { assert(false); }
    }
}

static void im_db_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_db *db = state;
    (void) chunk;

    db->id = id;
    db->len = im_db_len(id_item(id));
}

static void im_db_make(
        void *state, struct chunk *chunk, id_t id, const word_t *data, size_t len)
{
    struct im_db *db = state;
    im_db_init(db, chunk, id);

    if (len < 1) return;

    len = legion_min(len, im_db_len(id_item(id)));
    for (size_t i = 0; i < len; ++i)
        db->data[i] = data[i];
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_db_io_status(struct im_db *db, struct chunk *chunk, id_t src)
{
    word_t value = db->len;
    chunk_io(chunk, IO_STATE, db->id, src, &value, 1);
}

static void im_db_io_get(
        struct im_db *db, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    word_t val = 0;
    if (len >= 1) {
        word_t index = args[0];
        if (index >= 0 && index < db->len) val = db->data[index];
    }
    chunk_io(chunk, IO_VAL, db->id, src, &val, 1);
}

static void im_db_io_set(struct im_db *db, const word_t *args, size_t len)
{
    if (len < 2) return;
    word_t index = args[0];
    if (index < db->len) db->data[index] = args[1];
}

static void im_db_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_db *db = state;

    switch (io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, db->id, src, NULL, 0); return; }
    case IO_STATUS: { im_db_io_status(db, chunk, src); return; }

    case IO_GET: { im_db_io_get(db, chunk, src, args, len); return; }
    case IO_SET: { im_db_io_set(db, args, len); return; }

    default: { return; }
    }
}

static const word_t im_db_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_GET,
    IO_SET,
};
