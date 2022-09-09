/* config.c
   Rémi Attab (remi.attab@gmail.com), 02 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "utils/config.h"
#include "vm/atoms.h"


// -----------------------------------------------------------------------------
// reader
// -----------------------------------------------------------------------------

static void reader_init(struct reader *reader, const char *path)
{
    reader->file = mfile_open(path);
    reader->ctx = token_init_stderr(
            &reader->tok, path, reader->file.ptr, reader->file.len);
}

static void reader_free(struct reader *reader)
{
    mfile_close(&reader->file);
    if (!token_ctx_ok(reader->ctx)) exit(1);
    token_ctx_free(reader->ctx);
}

enum token_type reader_peek(struct reader *reader)
{
    struct token token = {0};
    return token_peek(&reader->tok, &token)->type;
}

bool reader_peek_close(struct reader *reader)
{
    return reader_peek(reader) == token_close;
}

bool reader_peek_eof(struct reader *reader)
{
    return reader_peek(reader) == token_nil;
}

void reader_open(struct reader *reader)
{
    struct token token = {0};
    token_expect(&reader->tok, &token, token_open);
}

void reader_close(struct reader *reader)
{
    struct token token = {0};
    token_expect(&reader->tok, &token, token_close);
}

uint64_t reader_u64(struct reader *reader)
{
    return reader_word(reader);
}

vm_word reader_word(struct reader *reader)
{
    struct token token = {0};
    if (!token_expect(&reader->tok, &token, token_number)) return 0;
    return token.value.w;
}

vm_word reader_atom(struct reader *reader, struct atoms *atoms)
{
    vm_word ret = 0;
    struct token token = {0};

    switch (token_next(&reader->tok, &token)->type)
    {
    case token_atom: { ret = atoms_get(atoms, &token.value.s); break; }
    case token_atom_make: { ret = atoms_make(atoms, &token.value.s); break; }
    default: {
        token_err(&reader->tok, "unexpected token '%s' != 'atom'",
                token_type_str(token.type));
        break;
    }
    }

    if (!ret) token_err(&reader->tok, "unknown atom '%s'", token.value.s.c);
    return ret;
}

struct symbol reader_symbol(struct reader *reader)
{
    struct token token = {0};
    return token_expect(&reader->tok, &token, token_symbol) ?
        token.value.s : (struct symbol) {0};
}

hash reader_symbol_hash(struct reader *reader)
{
    struct token token = {0};
    if (!token_expect(&reader->tok, &token, token_symbol)) return 0;
    return symbol_hash(&token.value.s);
}

void reader_expect(struct reader *reader, hash hash)
{
    struct symbol key = reader_symbol(reader);
    if (likely(hash == symbol_hash(&key))) return;

    token_err(&reader->tok, "unexpected field key '%s'", key.c);
}


// -----------------------------------------------------------------------------
// writer
// -----------------------------------------------------------------------------

enum { writer_chunks = s_page_len };

static void writer_out(struct writer *writer, char val);

static void writer_init(struct writer *writer, const char *path)
{
    writer->path = path;

    writer->fd = file_create_tmp(path, writer_chunks);
    if (writer->fd < 0) abort();

    const size_t cap = writer_chunks;
    writer->base = mmap(0, cap, PROT_WRITE, MAP_SHARED, writer->fd, 0);
    if (writer->base == MAP_FAILED)
        failf_errno("unable to mmap config tmp file '%s'", path);

    writer->end = writer->base + cap;
    writer->it = writer->base;
}

static void writer_free(struct writer *writer)
{
    writer_out(writer, '\n');

    size_t len = writer->it - writer->base;
    if (ftruncate(writer->fd, len) == -1)
        failf_errno("unable to truncate '%d' to '%zu'", writer->fd, len);

    if (msync(writer->base, len, MS_SYNC) == -1)
        failf_errno("unable to msync '%d'", writer->fd);

    if (munmap(writer->base, writer->end - writer->base) == -1)
        failf_errno("unable to unmap writer for '%s'", writer->path);
    close(writer->fd);

    file_tmp_swap(writer->path);
}

static void writer_ensure(struct writer *writer, size_t len)
{
    if (likely(writer->it + len <= writer->end)) return;

    const size_t cap_old = writer->end - writer->base;;
    const size_t len_old = writer->it - writer->base;
    const size_t len_new = len_old + len;
    assert(len_new > cap_old);

    size_t cap = cap_old;
    while (len_new >= cap) cap += writer_chunks;

    if (writer->fd && ftruncate(writer->fd, cap) == -1)
        failf_errno("unable to grow config file '%lx'", cap);

    writer->base = mremap(writer->base, cap_old, cap, MREMAP_MAYMOVE);
    if (writer->base == MAP_FAILED) {
        failf_errno("unable to remap '%p' from '%zu' to '%zu'",
                writer->base, cap_old, cap);
    }

    writer->it = writer->base + len_old;
    writer->end = writer->base + cap;
}

static void writer_write(struct writer *writer, const char *src, size_t len)
{
    writer_ensure(writer, len);
    memcpy(writer->it, src, len);
    writer->it += len;
}

static void writer_fill(struct writer *writer, char val, size_t len)
{
    writer_ensure(writer, len);
    memset(writer->it, val, len);
    writer->it += len;
}

static void writer_out(struct writer *writer, char val)
{
    writer_fill(writer, val, 1);
}

static char writer_prev(struct writer *writer)
{
    return writer->it == writer->base ? 0 : *(writer->it - 1);
}

static void writer_space(struct writer *writer)
{
    switch (writer_prev(writer)) {
    case '\0': case '\n': case ' ': case '(': { break; }
    default: { writer_out(writer, ' '); }
    }
}

void writer_line(struct writer *writer)
{
    writer_out(writer, '\n');
    writer_fill(writer, ' ', writer->depth * 2);
}

void writer_open(struct writer *writer)
{
    writer_space(writer);
    writer_out(writer, '(');
    writer->depth++;
}

void writer_open_nl(struct writer *writer)
{
    writer_line(writer);
    writer_open(writer);
}

void writer_close(struct writer *writer)
{
    writer_out(writer, ')');
    writer->depth--;
}

void writer_u64(struct writer *writer, uint64_t val)
{
    char str[2 + 16] = { '0', 'x' };
    size_t len = str_utox(val, str + 2, sizeof(str) - 2);
    assert(len + 2 == sizeof(str));

    writer_space(writer);
    writer_write(writer, str, sizeof(str));
}

void writer_word(struct writer *writer, vm_word val)
{
    char str[21] = {0};
    size_t len = str_utoa(val, str, sizeof(str));
    assert(len == sizeof(str));

    writer_space(writer);
    writer_write(writer, str, sizeof(str));
}

void writer_symbol(struct writer *writer, const struct symbol *val)
{
    writer_space(writer);
    writer_write(writer, val->c, val->len);
}

void writer_atom(struct writer *writer, const struct symbol *val)
{
    writer_space(writer);
    writer_out(writer, '!');
    writer_write(writer, val->c, val->len);
}

void writer_atom_fetch(struct writer *writer, struct atoms *atoms, vm_word val)
{
    struct symbol symbol = {0};
    if (!atoms_str(atoms, val, &symbol))
        failf("unknown atom for value '%lx'", val);

    writer_atom(writer, &symbol);
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

struct reader *config_read(struct config *config, const char *path)
{
    config->read = true;
    strncpy(config->path, path, sizeof(config->path) - 1);
    reader_init(&config->impl.read, config->path);
    return &config->impl.read;
}

struct writer *config_write(struct config *config, const char *path)
{
    config->read = false;
    strncpy(config->path, path, sizeof(config->path) - 1);
    writer_init(&config->impl.write, config->path);
    return &config->impl.write;
}

void config_close(struct config *config)
{
    if (config->read)
        reader_free(&config->impl.read);
    else writer_free(&config->impl.write);
}
