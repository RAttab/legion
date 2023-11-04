/* ast.c
   Rémi Attab (remi.attab@gmail.com), 06 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// ast_type
// -----------------------------------------------------------------------------

const char *ast_type_str(enum ast_type type)
{
    switch (type)
    {
    case ast_nil: { return "nil"; }

    case ast_reg: { return "reg"; }
    case ast_atom: { return "atom"; }
    case ast_number: { return "number"; }

    case ast_ref: { return "ref"; }
    case ast_decl: { return "decl"; }
    case ast_keyword: { return "keyword"; }

    case ast_fn: { return "fn"; }
    case ast_list: { return "list"; }
    case ast_tuple: { return "tuple"; }
    case ast_close: { return "close"; }

    case ast_comment: { return "comment"; }
    case ast_eof: { return "eof"; }

    default: { return "???"; }
    }
}


// -----------------------------------------------------------------------------
// ast
// -----------------------------------------------------------------------------

struct ast
{
    struct
    {
        size_t len, cap;
        struct ast_node *list;
    } nodes;

    struct
    {
        size_t len, cap;
        struct ast_log *list;
    } log;
};

struct ast *ast_alloc(void)
{
    return calloc(1, sizeof(struct ast));
}

void ast_free(struct ast *ast)
{
    if (ast->nodes.list) free(ast->nodes.list);
    if (ast->log.list) free(ast->log.list);
    free(ast);
}

ast_it ast_end(const struct ast *ast)
{
    // - 1 to get the eof node.
    return ast->nodes.list + ast->nodes.len - 1;
}

ast_it ast_begin(const struct ast *ast)
{
    return ast->nodes.list;

}

ast_it ast_at(const struct ast *ast, uint32_t pos)
{
    ast_it it = ast_begin(ast);
    ast_it end = ast_end(ast);
    while (it < end && it->pos > pos) ++it;
    return it;
}

ast_it ast_next(const struct ast *ast, ast_it it, uint32_t pos)
{
    ast_it end = ast_end(ast);
    assert(it <= end);

    while (it < end && pos >= (it + 1)->pos) ++it;
    return it;
}

ast_log_it ast_log_begin(const struct ast *ast)
{
    return ast->log.len ? ast->log.list : nullptr;
}

ast_log_it ast_log_end(const struct ast *ast)
{
    return ast->log.len ? ast->log.list + ast->log.len : nullptr;
}

ast_log_it ast_log_next(const struct ast *ast, uint32_t pos, ast_log_it it)
{
    ast_log_it end = ast_log_end(ast);
    assert(it <= end);

    while (it < end && pos >= (it + 1)->pos) it++;
    return it;
}


// -----------------------------------------------------------------------------
// parse - utils
// -----------------------------------------------------------------------------

struct ast_parser
{
    struct ast *ast;

    struct tokenizer tok;
    struct token token;

    const char *str;
    size_t len;
};

typedef void (*ast_parser_fn) (struct ast_parser *, uint32_t);

static struct
{
    bool init;
    struct hset *keywords;
    struct htable parsers;
} ast_data = {0};

static void ast_err_fn(void *ctx, const char *fmt, ...)
{
    struct ast_parser *parser = ctx;
    struct ast *ast = parser->ast;

    // + 1 -> we need an eof sentinel node at the end of the log.
    if (unlikely(ast->log.len + 1 >= ast->log.cap)) {
        size_t old = legion_xchg(
                &ast->log.cap,
                ast->log.cap ? ast->log.cap * 2 : 16);

        ast->log.list = realloc_zero(
                ast->log.list,
                old, ast->log.cap,
                sizeof(*ast->log.list));
    }

    struct ast_log *log = ast->log.list + ast->log.len++;
    log->pos = parser->token.pos;
    log->len = parser->token.len;

    va_list args = {0};
    va_start(args, fmt);
    (void) vsnprintf(log->msg, sizeof(log->msg), fmt, args);
    va_end(args);

    /* struct rowcol rc = rowcol(parser->str, log->pos); */
    /* dbgf("ast.err: %u:%u:%s", rc.row, rc.col, log->msg); */
}

static struct ast_node *ast_append_node(struct ast *ast)
{
    if (unlikely(ast->nodes.len == ast->nodes.cap)) {
        size_t old = legion_xchg(
                &ast->nodes.cap, ast->nodes.cap ? ast->nodes.cap * 2 : 1024);
        ast->nodes.list = realloc_zero(
                ast->nodes.list, old, ast->nodes.cap, sizeof(*ast->nodes.list));
    }

    return ast->nodes.list + ast->nodes.len++;
}

static uint32_t ast_index(struct ast_parser *parser, struct ast_node *node)
{
    return node - parser->ast->nodes.list;
}

// -----------------------------------------------------------------------------
// parser
// -----------------------------------------------------------------------------

static bool ast_parse_one(struct ast_parser *, uint32_t);
static bool ast_parse_until_close(struct ast_parser *, uint32_t);

static struct ast_node *ast_last(struct ast_parser *parser)
{
    return parser->ast->nodes.list + (parser->ast->nodes.len - 1);
}

static struct ast_node *ast_append_type(
        struct ast_parser *parser, enum ast_type type, uint32_t parent)
{
    struct ast_node *node = ast_append_node(parser->ast);
    *node = (struct ast_node) {
        .type = type,
        .pos = parser->token.pos,
        .len = parser->token.len,
        .parent = parent,
        .hash = 0,
    };

    /* dbgf("ast.append: type=%s, node=%lu, parent=%u, pos=%u, len=%u", */
    /*         ast_type_str(node->type), */
    /*         node - parser->ast->nodes.list, */
    /*         node->parent, node->pos, node->len); */

    return node;
}

static void ast_hash_node(struct ast_parser *parser, struct ast_node *node)
{
    assert(node->pos + node->len <= parser->len);
    node->hash = hash_bytes(hash_init(), parser->str + node->pos, node->len);
}

static void ast_parse_fn_defconst(struct ast_parser *parser, uint32_t parent)
{
    if (!token_expect(&parser->tok, &parser->token, token_symbol)) {
        token_goto_close(&parser->tok, &parser->token);
        return;
    }
    ast_hash_node(parser, ast_append_type(parser, ast_decl, parent));

    if (!ast_parse_one(parser, parent)) {
        token_err(&parser->tok, "missing closing bracket");
        return;
    }

    if (!token_expect(&parser->tok, &parser->token, token_close))
        token_goto_close(&parser->tok, &parser->token);
    else ast_append_type(parser, ast_close, parent);
}

static void ast_parse_fn_defun(struct ast_parser *parser, uint32_t parent)
{
    { // fn name
        if (!token_expect(&parser->tok, &parser->token, token_symbol)) {
            token_goto_close(&parser->tok, &parser->token);
            return;
        }
        ast_hash_node(parser, ast_append_type(parser, ast_decl, parent));
    }

    { // param list
        if (!token_expect(&parser->tok, &parser->token, token_open)) {
            token_goto_close(&parser->tok, &parser->token);
            return;
        }

        struct ast_node *list = ast_append_type(parser, ast_list, parent);
        uint32_t list_node = ast_index(parser, list);

        while (token_next(&parser->tok, &parser->token)->type == token_symbol)
            ast_hash_node(parser, ast_append_type(parser, ast_decl, list_node));

        if (!token_assert(&parser->tok, &parser->token, token_close)) {
            token_goto_close(&parser->tok, &parser->token);
            return;
        }
        ast_append_type(parser, ast_close, list_node);
    }

    // body
    {
        if (!ast_parse_until_close(parser, parent))
            token_err(&parser->tok, "missing closing bracket");
    }
}

static void ast_parse_fn_let(struct ast_parser *parser, uint32_t parent)
{
    if (!token_expect(&parser->tok, &parser->token, token_open)) {
        token_goto_close(&parser->tok, &parser->token);
        return;
    }
    struct ast_node *list = ast_append_type(parser, ast_list, parent);
    uint32_t list_node = ast_index(parser, list);

    while (token_next(&parser->tok, &parser->token)->type == token_open) {
        struct ast_node *tuple = ast_append_type(parser, ast_tuple, list_node);
        uint32_t tuple_node = ast_index(parser, tuple);

        if (!token_expect(&parser->tok, &parser->token, token_symbol)) {
            token_goto_close(&parser->tok, &parser->token);
            continue;
        }
        ast_hash_node(parser, ast_append_type(parser, ast_decl, tuple_node));

        if (!ast_parse_until_close(parser, tuple_node)) {
            token_err(&parser->tok, "missing closing bracket");
            return;
        }
    }

    if (!token_assert(&parser->tok, &parser->token, token_close)) {
        token_goto_close(&parser->tok, &parser->token);
        return;
    }
    ast_append_type(parser, ast_close, list_node);

    if (!ast_parse_until_close(parser, parent))
        token_err(&parser->tok, "missing closing bracket");
}

static void ast_parse_fn_for(struct ast_parser *parser, uint32_t parent)
{
    { // decl
        if (!token_expect(&parser->tok, &parser->token, token_open)) {
            token_goto_close(&parser->tok, &parser->token);
            return;
        }

        struct ast_node *tuple = ast_append_type(parser, ast_tuple, parent);
        uint32_t tuple_node = ast_index(parser, tuple);

        if (!token_expect(&parser->tok, &parser->token, token_symbol)) {
            token_goto_close(&parser->tok, &parser->token);
            goto predicate;
        }
        ast_hash_node(parser, ast_append_type(parser, ast_decl, tuple_node));

        if (!ast_parse_one(parser, tuple_node)) {
            token_goto_close(&parser->tok, &parser->token);
            goto predicate;
        }

        if (!token_expect(&parser->tok, &parser->token, token_close)) {
            token_goto_close(&parser->tok, &parser->token);
            goto predicate;
        }
        ast_append_type(parser, ast_close, tuple_node);
    }

  predicate:
    if (!ast_parse_one(parser, parent)) {
        token_err(&parser->tok, "missing for-loop predicate-clause");
        return;
    }

    // inc
    if (!ast_parse_one(parser, parent)) {
        token_err(&parser->tok, "missing for-loop increment-clause");
        return;
    }

    if (!ast_parse_until_close(parser, parent))
        token_err(&parser->tok, "missing closing bracket");
}

static void ast_parse_fn_case(struct ast_parser *parser, uint32_t parent)
{
    if (!ast_parse_one(parser, parent)) {
        token_err(&parser->tok, "missing case value");
        token_goto_close(&parser->tok, &parser->token);
        return;
    }

    if (!token_expect(&parser->tok, &parser->token, token_open)) {
        token_goto_close(&parser->tok, &parser->token);
        return;
    }

    struct ast_node *list = ast_append_type(parser, ast_list, parent);
    uint32_t list_node = ast_index(parser, list);

    while (token_next(&parser->tok, &parser->token)->type == token_open) {
        struct ast_node *tuple = ast_append_type(parser, ast_tuple, list_node);
        uint32_t tuple_node = ast_index(parser, tuple);

        if (!ast_parse_one(parser, list_node)) {
            token_err(&parser->tok, "missing test value for case-clause");
            return;
        }

        if (!ast_parse_one(parser, tuple_node)) {
            token_err(&parser->tok, "missing statement for case-clause");
            return;
        }

        if (token_expect(&parser->tok, &parser->token, token_close))
            ast_append_type(parser, ast_close, tuple_node);
        else token_goto_close(&parser->tok, &parser->token);
    }

    if (token_assert(&parser->tok, &parser->token, token_close))
        ast_append_type(parser, ast_close, list_node);
    else token_goto_close(&parser->tok, &parser->token);

    token_next(&parser->tok, &parser->token);

    // Default clause
    if (parser->token.type == token_open) {
        struct ast_node *tuple = ast_append_type(parser, ast_tuple, parent);
        uint32_t tuple_node = ast_index(parser, tuple);

        if (!token_expect(&parser->tok, &parser->token, token_symbol)) {
            token_goto_close(&parser->tok, &parser->token);
            token_goto_close(&parser->tok, nullptr);
            return;
        }

        ast_hash_node(parser, ast_append_type(parser, ast_decl, tuple_node));

        if (!ast_parse_one(parser, tuple_node)) {
            token_err(&parser->tok, "missing default-clause");
            token_goto_close(&parser->tok, &parser->token);
            token_goto_close(&parser->tok, nullptr);
            return;
        }

        if (token_expect(&parser->tok, &parser->token, token_close))
            ast_append_type(parser, ast_close, tuple_node);
        else token_goto_close(&parser->tok, &parser->token);

        token_next(&parser->tok, &parser->token);
    }

    if (token_assert(&parser->tok, &parser->token, token_close))
        ast_append_type(parser, ast_close, parent);
    else token_goto_close(&parser->tok, &parser->token);
}

static void ast_parse_fn_mod(struct ast_parser *parser, uint32_t parent)
{
    struct token token = {0};
    if (token_peek(&parser->tok, &token)->type == token_symbol) {
        token_expect(&parser->tok, &parser->token, token_symbol);

        struct ast_node *name = ast_append_type(parser, ast_ref, parent);

        if (token_peek(&parser->tok, &parser->token)->type == token_sep) {
            token_expect(&parser->tok, &parser->token, token_sep);
            name->len++;

            if (!token_expect(&parser->tok, &parser->token, token_number)) {
                token_goto_close(&parser->tok, &parser->token);
                return;
            }

            name->len += parser->token.len;
        }

        ast_hash_node(parser, name);
    }

    if (token_expect(&parser->tok, &parser->token, token_close))
        ast_append_type(parser, ast_close, parent);
    else token_goto_close(&parser->tok, &parser->token);
}

static void ast_parse_fn(struct ast_parser *parser, uint32_t parent)
{
    struct ast_node *node = ast_last(parser);

    while (node->type != ast_keyword) {
        if (token_peek(&parser->tok, &parser->token)->type != token_sep) break;
        token_expect(&parser->tok, &parser->token, token_sep);
        token_next(&parser->tok, &parser->token);
        node->len++;

        if (parser->token.type == token_symbol) {
            node->len += parser->token.len;

            if (token_peek(&parser->tok, &parser->token)->type != token_sep) break;
            token_expect(&parser->tok, &parser->token, token_sep);
            token_next(&parser->tok, &parser->token);
            node->len++;
        }

        if (parser->token.type == token_number) {
            node->len += parser->token.len;
            break;
        }

        token_err(&parser->tok, "invalid function name");
        token_goto_close(&parser->tok, &parser->token);
        return;
    }

    if (node->type != ast_keyword) ast_hash_node(parser, node);

    if (!ast_parse_until_close(parser, parent))
        token_err(&parser->tok, "missing closing bracket");
}

static bool ast_parse_one(struct ast_parser *parser, uint32_t parent)
{
    token_next(&parser->tok, &parser->token);

    switch (parser->token.type)
    {

    case token_nil: { ast_append_type(parser, ast_eof, 0); return false; }

    case token_atom:
    case token_atom_make: { ast_hash_node(parser, ast_append_type(parser, ast_atom, parent)); break; }
    case token_symbol:    { ast_hash_node(parser, ast_append_type(parser, ast_ref, parent)); break; }

    case token_number:    { ast_append_type(parser, ast_number, parent); break; }
    case token_reg:       { ast_append_type(parser, ast_reg, parent); break; }
    case token_comment:   { ast_append_type(parser, ast_comment, parent); break; }

    case token_sep: { token_err(&parser->tok, "unexpected seperator"); break; }

    case token_open: {
        struct ast_node *open = ast_append_type(parser, ast_fn, parent);
        parent = ast_index(parser, open);

        struct token *token = token_expect(&parser->tok, &parser->token, token_symbol);
        if (!token) { token_goto_close(&parser->tok, &parser->token); break; }

        const hash_val hash = symbol_hash(&token->value.s);
        bool keyword = hset_test(ast_data.keywords, hash);
        ast_append_type(parser, keyword ? ast_keyword : ast_ref, parent);

        struct htable_ret ret = { .ok = false };
        if (keyword) ret = htable_get(&ast_data.parsers, hash);
        ast_parser_fn fn = ret.ok ? (ast_parser_fn) ret.value : ast_parse_fn;
        fn(parser, parent);

        break;
    }

    case token_close: { ast_append_type(parser, ast_close, parent); break; }

    default: { assert(false); }

    }
    return true;
}

static bool ast_parse_until_close(struct ast_parser *parser, uint32_t parent)
{
    assert(parent);
    assert(parent < parser->ast->nodes.len);

    struct token token = {0};
    while (token_peek(&parser->tok, &token)->type != token_close)
        if (!ast_parse_one(parser, parent)) return false;

    if (!token_expect(&parser->tok, &parser->token, token_close))
        token_goto_close(&parser->tok, &parser->token);
    else ast_append_type(parser, ast_close, parent);

    return true;
}

void ast_parse(struct ast *ast, const char *str, size_t len)
{
    struct ast_parser parser = { .ast = ast, .str = str, .len = len };

    token_init(&parser.tok, str, len,  ast_err_fn, &parser);
    token_comments(&parser.tok, true);

    ast->nodes.len = 0;
    ast->log.len = 0;

    // First node in the tree is nil so we can refer to parent 0 without
    // pointing to an actual thing.
    *ast_append_node(ast) = (struct ast_node) { .type = ast_nil };

    while (ast_parse_one(&parser, 0));

    // If we have errors then setup our sentinel node at the end of the list.
    if (ast->log.len) {
        struct ast_log *log = ast->log.list + ast->log.len;
        log->pos = parser.token.pos;
    }
}

bool ast_step(const char *in, size_t len, struct ast_node *out)
{
    assert(out->pos + out->len <= len);

    const char *it = in + out->pos + out->len;
    const char *end = in + len;
    memset(out, 0, sizeof(*out));

    it += str_skip_spaces(it, end - it);
    if (it == end) return false;
    out->pos = it - in;

    switch (*it)
    {
    case ';': {
        out->len = end - it;
        out->type = ast_comment;
        break;
    }

    case '(': case ')': {
        while (it < end && (*it == '(' || *it == ')')) ++it, ++out->len;
        break;
    }

    case '?': case '!': {
        while (it < end && !str_is_space(*it) && *it != '(' && *it != ')')
            ++it, ++out->len;
        out->type = ast_atom;
        break;
    }

    default: {
        while (it < end && !str_is_space(*it) && *it != '(' && *it != ')')
            ++it, ++out->len;

        hash_val hash = hash_bytes(hash_init(), in + out->pos, out->len);
        if (hset_test(ast_data.keywords, hash))
            out->type = ast_keyword;

        break;
    }

    }

    /* dbgf("ast.step: len=%zu, out=%u:%u:%s, str=%.*s", */
    /*         len, out->pos, out->len, ast_type_str(out->type), */
    /*         (unsigned) out->len, in + out->pos); */

    return true;
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

void ast_populate(void)
{
    if (ast_data.init) return;
    ast_data.init = true;

    void fn(const char *str, ast_parser_fn fn)
    {
        struct symbol sym = make_symbol(str);
        hash_val hash = symbol_hash(&sym);
        auto ret = htable_put(&ast_data.parsers, hash, (uintptr_t) fn);
        assert(ret.ok);
    }

    htable_reset(&ast_data.parsers);
    fn("defconst", ast_parse_fn_defconst);
    fn("defun", ast_parse_fn_defun);
    fn("let", ast_parse_fn_let);
    fn("for", ast_parse_fn_for);
    fn("case", ast_parse_fn_case);
    fn("mod", ast_parse_fn_mod);


    void keyword(const char *str)
    {
        struct symbol sym = make_symbol(str);
        ast_data.keywords = hset_put(ast_data.keywords, symbol_hash(&sym));
    }

    ast_data.keywords = hset_reserve(128);

    keyword("asm");
    keyword("@");
#define vm_op_fn(op, str, arg) keyword(#str);
#include "opx.h"

    keyword("defun");
    keyword("load");
    keyword("mod");

    keyword("head");
    keyword("progn");
    keyword("defconst");
    keyword("let");
    keyword("if");
    keyword("case");
    keyword("when");
    keyword("unless");
    keyword("while");
    keyword("for");

    keyword("set");
    keyword("id");
    keyword("self");
    keyword("io");
    keyword("ior");

    keyword("not");
    keyword("and");
    keyword("xor");
    keyword("or");
    keyword("bnot");
    keyword("band");
    keyword("bxor");
    keyword("bor");
    keyword("bsl");
    keyword("bsr");

    keyword("rem");
    keyword("lmul");
    keyword("cmp");

    keyword("reset");
    keyword("yield");
    keyword("tsc");
    keyword("fault");
    keyword("assert");
    keyword("specs");

    keyword("pack");
    keyword("unpack");
}


// -----------------------------------------------------------------------------
// dump
// -----------------------------------------------------------------------------

void ast_dump(const struct ast *ast)
{
    dbgf("ast.nodes: %zu:%zu", ast->nodes.len, ast->nodes.cap);
    for (size_t i = 0; i < ast->nodes.len; ++i) {
        ast_it it = ast->nodes.list + i;
        dbgf("  [%04zu] type=%-7s pos=%04u len=%04u parent=%04u",
                i, ast_type_str(it->type), it->pos, it->len, it->parent);
    }

    if (!ast->log.len) return;

    dbgf("ast.logs: %zu:%zu", ast->log.len, ast->log.cap);
    for (size_t i = 0; i < ast->log.len; ++i) {
        ast_log_it it = ast->log.list + i;
        dbgf("  [%04zu] pos=%04u len=%04u msg=%s",
                i, it->pos, it->len, it->msg);
    }
}
