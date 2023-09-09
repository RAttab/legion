/* ast.h
   Rémi Attab (remi.attab@gmail.com), 06 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// ast
// -----------------------------------------------------------------------------

enum ast_type
{
    ast_nil = 0,

    ast_reg,
    ast_atom,
    ast_number,

    ast_ref,
    ast_decl,
    ast_keyword,

    ast_fn,
    ast_list, // variable length
    ast_tuple, // fixed length
    ast_close,

    ast_comment,
    ast_eof,
};

const char *ast_type_str(enum ast_type);

struct ast_node
{
    enum ast_type type;
    uint32_t pos, len;
    uint32_t parent;
    hash_val hash;
};
typedef const struct ast_node *ast_it;

struct ast;

struct ast *ast_alloc(void);
void ast_free(struct ast *);

ast_it ast_end(const struct ast *);
ast_it ast_begin(const struct ast *);
ast_it ast_at(const struct ast *, uint32_t);
ast_it ast_next(const struct ast *, ast_it, uint32_t);

constexpr size_t ast_log_cap = 128;
struct ast_log { uint32_t pos, len; char msg[ast_log_cap]; };
typedef const struct ast_log *ast_log_it;
ast_log_it ast_log_begin(const struct ast *);
ast_log_it ast_log_end(const struct ast *);
ast_log_it ast_log_next(const struct ast *, uint32_t pos, ast_log_it it);

inline bool ast_log_in(ast_log_it it, uint32_t pos)
{
    return pos >= it->pos && pos < it->pos + it->len;
}

void ast_parse(struct ast *, const char *str, size_t len);

void ast_dump(const struct ast *);

void ast_populate(void);
