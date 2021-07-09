/* program.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "prog.h"


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
    prog_id_t id;
    enum item host;
    uint8_t inputs, outputs;
    enum item tape[];
};


prog_id_t prog_id(const struct prog *prog) { return prog->id; }
enum item prog_host(const struct prog *prog) { return prog->host; }
size_t prog_len(const struct prog *prog) { return prog->inputs + prog->outputs; }

struct prog_ret prog_at(const struct prog *prog, prog_it_t it)
{
    struct prog_ret ret = { .state = prog_eof };

    if (it < prog->inputs) {
        ret.state = prog_input;
        ret.item = prog->tape[it];
    }
    else if (it < prog->inputs + prog->outputs) {
        ret.state = prog_output;
        ret.item = prog->tape[it];
    }

    return ret;
}


// -----------------------------------------------------------------------------
// parsing
// -----------------------------------------------------------------------------

static struct
{
    struct prog *index[ITEM_MAX];
} progs;


const struct prog *prog_fetch(prog_id_t prog)
{
    return progs.index[prog];
}

static const char *prog_hex(
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

static const char *prog_hex8(const char *it, const char *end, uint8_t *ret)
{
    uint64_t value = 0;
    it = prog_hex(it, end, &value, sizeof(*ret));
    *ret = value;
    return it;
}

static const char *prog_expect(const char *it, const char *end, char exp)
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
    assert(line_len > 5);
    assert(line_len % 3 == 0);

    size_t tape_len = (line_len / 3) - 2;
    assert(tape_len >= 1);

    struct prog *prog = calloc(1, sizeof(*prog) + tape_len * sizeof(prog->tape[0]));

    it = prog_hex8(it, end, &prog->host);
    it = prog_expect(it, end, ':');
    it = prog_hex8(it, end, &prog->id);

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

        it = prog_hex8(it+1, end, &prog->tape[index]);
        index++;
    }

  out:
    assert(state != prog_eof);
    assert(prog->inputs + prog->outputs > 0);
    return prog;
}


void prog_load()
{
    char path[PATH_MAX] = {0};
    core_path_res("progs.db", path, sizeof(path));

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
            struct prog *prog = prog_read(line, it);
            assert(prog);

            assert(!progs.index[prog->id]);
            progs.index[prog->id] = prog;
        }
        it++;
    }

    munmap(base, len);
    close(fd);
}
