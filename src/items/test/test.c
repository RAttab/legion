/* test.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

static void im_test_init(void *state, struct chunk *chunk, im_id id)
{
    (void) state, (void) chunk, (void) id;
}

bool im_test_check(
        const struct im_test *test,
        enum io io, im_id src, const vm_word *args, size_t len)
{
    bool ok = true;

    if (test->io != io) {
        dbgf("im.test.io: %x != %x", test->io, io);
        ok = false;
    }

    if (test->src != src) {
        dbgf("im.test.src: %x != %x", test->src, src);
        ok = false;
    }

    if (test->len != len) {
        dbgf("im.test.len: %x != %zu", test->len, len);
        ok = false;
    }

    for (size_t i = 0; i < len; ++i) {
        if (test->args[i] != args[i]) {
            dbgf("im.test.args[%zu]: %lx != %lx", i, test->args[i], args[i]);
            ok = false;
        }
    }

    return ok;
}

static void im_test_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_test *test = state;
    (void) chunk;

    test->io = io;
    test->src = src;
    test->len = len;
    memcpy(test->args, args, len * sizeof(*args));
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

void im_test_config(struct im_config *config)
{
    config->size = sizeof(struct im_test);

    config->im.init = im_test_init;
    config->im.io = im_test_io;
}
