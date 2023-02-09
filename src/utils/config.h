/* config.h
   Rémi Attab (remi.attab@gmail.com), 01 Jan 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/token.h"
#include "utils/fs.h"

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

bool reader_goto_close(struct reader *);

enum token_type reader_peek(struct reader *);
bool reader_peek_close(struct reader *);
bool reader_peek_eof(struct reader *);

void reader_open(struct reader *);
void reader_close(struct reader *);
uint64_t reader_u64(struct reader *);
vm_word reader_word(struct reader *);
vm_word reader_atom(struct reader *, struct atoms *);
struct symbol reader_atom_symbol(struct reader *);

struct symbol reader_symbol(struct reader *);
hash_val reader_symbol_hash(struct reader *);

struct reader_table { const char *str; hash_val hash; uint64_t value; };
uint64_t reader_symbol_table(struct reader *, struct reader_table *, size_t len);

void reader_expect(struct reader *reader, hash_val);
#define reader_symbol_str(_reader, _str) \
    do { reader_expect((_reader), symbol_hash_c(_str)); } while (false)

#define reader_err(_reader, _fmt, ...)            \
    do { token_err(&((_reader)->tok), _fmt, __VA_ARGS__); } while (false)

// This might be a little overboard for a macro...
#define reader_field(_reader, _key, type)                               \
    ({                                                                  \
        struct reader *_in = (_reader);                                 \
        reader_open(_in);                                               \
                                                                        \
        struct symbol _sym = reader_symbol(_in);                        \
        if (symbol_hash(&_sym) != symbol_hash_c(_key))                  \
            reader_err(_in, "unexpected field key '%s'", _sym.c);       \
                                                                        \
        typeof(reader_ ## type(_in)) _ret = reader_ ## type(_in);       \
        reader_close(_in);                                              \
        _ret;                                                           \
    })

#define reader_field_atom(_reader, _key, _atoms)                        \
    ({                                                                  \
        struct reader *_in = (_reader);                                 \
        reader_open(_in);                                               \
                                                                        \
        struct symbol _sym = reader_symbol(_in);                        \
        if (symbol_hash(&_sym) != symbol_hash_c(_key))                  \
            reader_err(_in, "unexpected field key '%s'", _sym.c);       \
                                                                        \
        word _ret = reader_atom (_in, _atoms);                          \
        reader_close(_in);                                              \
        _ret;                                                           \
    })


// -----------------------------------------------------------------------------
// writer
// -----------------------------------------------------------------------------

struct writer
{
    int fd;
    const char *path;

    char *base, *it, *end;
    size_t depth;
};

void writer_line(struct writer *);
void writer_open(struct writer *);
void writer_open_nl(struct writer *);
void writer_close(struct writer *);
void writer_u64(struct writer *, uint64_t);
void writer_word(struct writer *, vm_word);
void writer_atom_fetch(struct writer *, struct atoms *, vm_word);
void writer_atom(struct writer *, const struct symbol *);
void writer_symbol(struct writer *, const struct symbol *);

#define writer_symbol_str(_writer, _str)                                \
    do {                                                                \
        static_assert(__builtin_constant_p(_str));                      \
        struct symbol _sym = make_symbol_len((_str), sizeof(_str));     \
        writer_symbol((_writer), &_sym);                                \
    } while (false)

#define writer_field(_writer, _key, type, _val) \
    do {                                        \
        struct writer *_out = (_writer);        \
        writer_open_nl(_out);                   \
        writer_symbol_str(_out, (_key));        \
        writer_ ## type(_out, (_val));          \
        writer_close(_out);                     \
    } while (false)

#define writer_field_atom(_writer, _key, _atoms, _val)  \
    do {                                                \
        struct writer *_out = (_writer);                \
        writer_open_nl(_out);                           \
        writer_symbol_str(_out, (_key));                \
        writer_atom_fetch(_out, (_atoms), (_val));      \
        writer_close(_out);                             \
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
