/* program.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "program.h"


#include "render/core.h"
#include "utils/htable.h"
#include "utils/str.h"
#include "utils/log.h"

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


// -----------------------------------------------------------------------------
// prog
// -----------------------------------------------------------------------------

struct prog
{
    uint8_t id;
    item_t host;
    uint8_t inputs, outputs;
    item_t tape[];
};


prog_id_t prog_id(const struct prog *prog) { return prog->id; }
item_t prog_host(const struct prog *prog) { return prog->host; }


struct prog_it prog_begin(const struct prog *prog)
{
    return (struct prog_it) { .prog = prog, .index = 0 };
}


struct prog_ret prog_peek(struct prog_it *it)
{
    struct prog_ret ret = { .state = prog_eof };

    if (it->index < prog->inputs) {
        ret.state = prog_input;
        ret.item = prog->tape[it->index];
    }
    else if (it->index < prog->inputs + prog->outputs) {
        ret.state = prog_output;
        ret.item = prog->tape[it->index];
    }

    return ret;
}


struct prog_ret prog_next(struct prog_it *it)
{
    struct prog_ret ret = prog_peek(it);
    if (ret.state != prog_eof) index++;
    return ret;
}


// -----------------------------------------------------------------------------
// parsing
// -----------------------------------------------------------------------------

static struct
{
    struct htable index;
} progs;


const struct prog *prog_fetch(prog_id_t prog)
{
    struct htable_ret ret = htable_get(&progs.index, prog);
    if (!ret.ok) return NULL;
    return (const struct prog *) ret.value;
}


static char *prog_hex(const char *it, const char *end, uint8_t *ret)
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


static char *prog_expect(const char *it, const char *end, char exp)
{
    assert(it < end);
    assert(*it == exp);
    return it + 1;
}


// 00:00<00,00,00,00>00,00.
// 00:00<00,00,00,00.
// 00:00>00,00,00,00.
static struct prog *prog_read(const char *it, const char *end)
{
    size_t line_len = end - it;
    assert(line_len % 3 == 0);

    size_t tape_len = len / 3 - 2;
    assert(tape_len >= 1);

    struct prog *prog = calloc(1, sizeof(*prog) + tape_len * sizeof(prog->tape[0]));

    it = prog_hex(it, end, &prog->id);
    it = prog_expect(it, end, ':');
    it = prog_hex(it, end, &prog->host);

    size_t index = 0;
    enum prog_state state = prog_eof;

    while (it < end) {

        switch (*it) {
        case '<': {
            assert(state == prog_eof);
            state = prog_input;
            break;
        }
        case '>': {
            assert(state != prog_output);
            state = prog_output;
            prog->inputs = index;
            break;
        }
        case '.': {
            assert(state != prog_eof);
            prog->outputs = index - prog->inputs;
            goto out;
        }
        case ',': { assert(state != prog_eof); break; }
        default: { assert(false); }
        }

        it = prog_hex(it, end, &prog->tape[index]);
        index++;
    }

  out:
    assert(state != prog_eof);
    assert(prog->inputs + prog_outputs > 0);
    return prog;
}


void prog_load()
{
    char path[PATH_MAX] = {0};
    core_path_res("progs.db", path, sizeof(path));

    struct stat stat = {0};
    if (fstat(fd, &stat) < 0) fail_errno("failed to stat: %s", path);

    size_t len = stat.st_size;
    void *base = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) fail_errno("failed to mmap: %s", path);

    const char *it = base;
    const char *end = it + len;
    htable_reserve(&progs.index, UINT8_MAX);

    while (it < end) {
        const char *line = it;
        while (*it != '\n' && it < end) it++;

        if (it != line) {
            struct prog *prog = prog_read(line, it);
            assert(prog);

            struct htable_ret ret = htable_put(&progs.index, prog->id, (uint64_t) prog);
            assert(ret.ok);
        }
        it++;
    }

    munmap(base, len);
    close(fd);
}
