/* code.h
   Rémi Attab (remi.attab@gmail.com), 09 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct code;
struct code_str;

constexpr uint32_t code_pos_nil = UINT32_MAX;

struct code_it
{
    struct code_str *node;
    uint32_t pos, row, col;

    uint32_t len;
    const char *str;
    bool eol;

    struct {
        ast_it node; bool use_node;
        ast_log_it log; bool use_log;
    } ast;
};

inline ast_it code_it_ast_node(const struct code_it *it)
{
    return it->ast.use_node ? it->ast.node : nullptr;
}

inline ast_log_it code_it_ast_log(const struct code_it *it)
{
    return it->ast.use_log ? it->ast.log : nullptr;
}


struct code *code_alloc(void);
void code_free(struct code *);

bool code_empty(struct code *);
size_t code_len(struct code *);
hash_val code_hash(struct code *);

void code_reset(struct code *);
void code_set(struct code *, const char *, size_t);
void code_update(struct code *);

struct code_it code_begin(struct code *, uint32_t row);
bool code_step(struct code *, struct code_it *);
size_t code_next_cols(struct code *, struct code_it *);

char code_char_for(const struct code *, uint32_t pos);
uint32_t code_cols_for(const struct code *, uint32_t row);
struct rowcol code_rowcol(const struct code *);
struct rowcol code_rowcol_for(const struct code *, uint32_t pos);
uint32_t code_pos_for(const struct code *, uint32_t row, uint32_t col);
ast_it code_ast_node_for(const struct code *, uint32_t pos);

uint32_t code_undo(struct code *);
uint32_t code_redo(struct code *);

void code_insert(struct code *, uint32_t pos, char);
void code_insert_range(struct code *, uint32_t pos, const char *, size_t);

void code_delete(struct code *, uint32_t pos);
void code_delete_range(struct code *, uint32_t first, uint32_t last);

size_t code_write(struct code *, char *dst, size_t len);
size_t code_write_range(struct code *, uint32_t first, uint32_t last, char *dst, size_t len);

uint32_t code_indent(struct code *, uint32_t pos);
uint32_t code_indent_range(struct code *, uint32_t first, uint32_t last);

uint32_t code_move_row(struct code *, uint32_t pos, int32_t inc);
uint32_t code_move_col(struct code *, uint32_t pos, int32_t inc);
uint32_t code_move_token(struct code *, uint32_t pos, int32_t inc);
uint32_t code_move_paragraph(struct code *, uint32_t pos, int32_t inc);
uint32_t code_move_home(struct code *, uint32_t pos);
uint32_t code_move_end(struct code *, uint32_t pos);
uint32_t code_move_paren(struct code *, uint32_t pos);
uint32_t code_move_symbol(struct code *, uint32_t pos, int32_t inc);

uint32_t code_find(
        struct code *,
        uint32_t first, uint32_t last,
        const char *str, size_t len);

size_t code_replace(
        struct code *,
        uint32_t first, uint32_t last,
        const char *find, size_t find_len,
        const char *value, size_t value_len);

void code_dump(struct code *);
