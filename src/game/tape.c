/* tape.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "tape.h"


#include "render/core.h"
#include "utils/htable.h"
#include "utils/str.h"
#include "utils/log.h"

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct tape
{
    enum item id;
    enum item host;
    uint8_t inputs, outputs;
    enum item tape[];
};


enum item tape_id(const struct tape *tape) { return tape->id; }
enum item tape_host(const struct tape *tape) { return tape->host; }
size_t tape_len(const struct tape *tape) { return tape->inputs + tape->outputs; }

struct tape_ret tape_at(const struct tape *tape, tape_it_t it)
{
    struct tape_ret ret = { .state = tape_eof };

    if (it < tape->inputs) {
        ret.state = tape_input;
        ret.item = tape->tape[it];
    }
    else if (it < tape->inputs + tape->outputs) {
        ret.state = tape_output;
        ret.item = tape->tape[it];
    }

    return ret;
}


// -----------------------------------------------------------------------------
// parsing
// -----------------------------------------------------------------------------

static struct
{
    struct tape *index[ITEM_MAX];
} tapes;


const struct tape *tape_fetch(enum item tape)
{
    return tapes.index[tape];
}

static const char *tape_hex(
        const char *it, const char *end, uint64_t *ret, size_t bytes)
{
    size_t len = bytes * 2;
    assert(it + len < end);
    assert(len >= 2 && len <= 16);

    *ret = 0;
    do {
        uint8_t val = str_charhex(*it);
        assert(val != 0xFF);
        *ret = (*ret << 4) | val;
        it++;
    } while (--len);

    return it;
}

static const char *tape_hex8(const char *it, const char *end, uint8_t *ret)
{
    uint64_t value = 0;
    it = tape_hex(it, end, &value, sizeof(*ret));
    *ret = value;
    return it;
}

static const char *tape_expect(const char *it, const char *end, char exp)
{
    assert(it < end);
    assert(*it == exp);
    return it + 1;
}


// 00:00<00,00,00,00>00,00.
// 00:00<00,00,00,00.
// 00:00>00,00,00,00.
static struct tape *tape_read(const char *it, const char *end)
{
    size_t line_len = end - it;
    assert(line_len > 5);
    assert(line_len % 3 == 0);

    size_t tape_len = (line_len / 3) - 2;
    assert(tape_len >= 1);

    struct tape *tape = calloc(1, sizeof(*tape) + tape_len * sizeof(tape->tape[0]));

    it = tape_hex8(it, end, &tape->host);
    it = tape_expect(it, end, ':');
    it = tape_hex8(it, end, &tape->id);

    size_t index = 0;
    enum tape_state state = tape_eof;

    while (it < end) {
        switch (*it) {
        case '<': {
            assert(state == tape_eof);
            state = tape_input;
            break;
        }
        case '>': {
            assert(state != tape_output);
            state = tape_output;
            tape->inputs = index;
            break;
        }
        case '.': {
            assert(state != tape_eof);
            tape->outputs = index - tape->inputs;
            goto out;
        }
        case ',': { assert(state != tape_eof); break; }
        default: { assert(false); }
        }

        it = tape_hex8(it+1, end, &tape->tape[index]);
        index++;
    }

  out:
    assert(state != tape_eof);
    assert(tape->inputs + tape->outputs > 0);
    return tape;
}


void tape_load()
{
    char path[PATH_MAX] = {0};
    core_path_res("tapes.db", path, sizeof(path));

    int fd = open(path, O_RDONLY);
    if (fd == -1) fail_errno("failed to open: %s", path);

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

    size_t len = stat.st_size;
    void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

    const char *it = base;
    const char *end = it + len;
    while (it < end) {
        const char *line = it;
        while (*it != '\n' && it < end) it++;

        if (it != line) {
            struct tape *tape = tape_read(line, it);
            assert(tape);

            assert(!tapes.index[tape->id]);
            tapes.index[tape->id] = tape;
        }
        it++;
    }

    munmap(base, len);
    close(fd);
}
