/* config.h
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/fs.h"
#include "utils/token.h"

struct atoms;

// -----------------------------------------------------------------------------
// reader
// -----------------------------------------------------------------------------

struct reader
{
    struct mfile file;
    struct tokenizer tok;
    struct token_ctx *ctx;
};

enum token_type reader_peek(struct reader *);
bool reader_peek_close(struct reader *);

void reader_open(struct reader *);
void reader_close(struct reader *);
uint64_t reader_u64(struct reader *);
word_t reader_word(struct reader *);
word_t reader_atom(struct reader *, struct atoms *);

struct symbol reader_symbol(struct reader *);
uint64_t reader_symbol_hash(struct reader *);

void reader_expect(struct reader *reader, hash_t);
#define reader_key(reader, str) \
    do { reader_expect((reader), symbol_hash_c(str)); } while (false)

#define reader_err(reader, fmt, ...)            \
    do { token_err(&((reader)->tok), fmt, __VA_ARGS__); } while (false)

// This might be a little overboard for a macro...
#define reader_field(reader, key, type)                                 \
    ({                                                                  \
        struct reader *in = (reader);                                   \
        reader_open(in);                                                \
                                                                        \
        struct symbol sym = reader_symbol(in);                          \
        if (symbol_hash(sym) != symbol_hash_c(key))                     \
            reader_err("unexpected field key '%s'" sym.c);              \
                                                                        \
        typeof(reader_ ## type (in)) ret = reader_ ## type (in);        \
        reader_close(in);                                               \
        ret;                                                            \
    })

#define reader_field_atom(reader, key, atoms)                   \
    ({                                                          \
        struct reader *in = (reader);                           \
        reader_open(in);                                        \
                                                                \
        struct symbol sym = reader_symbol(in);                  \
        if (symbol_hash(sym) != symbol_hash_c(key))             \
            reader_err("unexpected field key '%s'" sym.c);      \
                                                                \
        word_t ret = reader_atom (in, atoms);                   \
        reader_close(in);                                       \
        ret;                                                    \
    })


// -----------------------------------------------------------------------------
// writer
// -----------------------------------------------------------------------------

struct writer
{
    int fd;
    const char *path;

    const char *base, *it, *end;
    size_t depth;
};

void writer_line(struct writer *);
void writer_open(struct writer *);
void writer_open_nl(struct writer *);
void writer_close(struct writer *);
void writer_u64(struct writer *, uint64_t);
void writer_word(struct writer *, word_t);
void writer_atom(struct writer *, struct atoms *, word_t);

void writer_symbol(struct writer *, const struct symbol *);

#define writer_symbol_str(writer, str)                                  \
    do {                                                                \
        static_assert(__builtin_constant_p(str));                       \
        struct symbol symbol = make_symbol_len((str), sizeof(str));     \
        writer_symbol((writer), &symbol);                               \
    } while (false)

#define writer_field(writer, key, type, val)    \
    do {                                        \
        struct writer *out = (writer);          \
        writer_open_nl(out);                    \
        writer_symbol_str(out, (key));          \
        writer_ ## type(out, (val));            \
        writer_close(out);                      \
    } while (false)

#define writer_field_atom(writer, key, atoms, val)      \
    do {                                                \
        struct writer *out = (writer);                  \
        writer_open_nl(out);                            \
        writer_symbol_str(out, (key));                  \
        writer_atom(out, (atoms), (val));               \
        writer_close(out);                              \
    } while (false)


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

struct config
{
    bool read;
    char path[PATH_MAX];

    union
    {
        struct reader read;
        struct writer write;
    } impl;
};

struct reader *config_read(struct config *, const char *path);
struct writer *config_write(struct config *, const char *path);
void config_close(struct config *);
