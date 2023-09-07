/* code.c
   Rémi Attab (remi.attab@gmail.com), 09 Aug 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/code.h"
#include "vm/ast.h"
#include "utils/str.h"

// -----------------------------------------------------------------------------
// code str
// -----------------------------------------------------------------------------

struct code_str
{
    char *str;
    uint32_t len, cap;
    struct { ast_it node; ast_log_it log; } ast;
};

static void code_str_reserve(struct code_str *str, size_t len)
{
    if (likely(len <= str->cap)) return;

    size_t old = str->cap;
    if (!old) str->cap = legion_max(len, 8U);
    while (str->cap < len) str->cap *= 2;

    str->str = realloc_zero(str->str, old, str->cap, sizeof(char));
}

static void code_str_append(struct code_str *str, const char *src, size_t len)
{
    code_str_reserve(str, str->len + len);
    memcpy(str->str + str->len, src, len);
    str->len += len;
}

static void code_str_set(struct code_str *str, const char *src, size_t len)
{
    str->len = 0;
    code_str_append(str, src, len);
}


static void code_str_put(struct code_str *str, uint32_t pos, char c)
{
    assert(pos <= str->len);
    code_str_reserve(str, str->len + 1);
    memmove(str->str + pos + 1, str->str + pos, str->len - pos);
    str->str[pos] = c;
    str->len++;
}

static void code_str_del(struct code_str *str, uint32_t pos)
{
    assert(pos < str->len);
    memmove(str->str + pos, str->str + pos + 1, str->len - pos - 1);
    str->len--;
}

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

static constexpr size_t code_str_cap = 32;

enum code_hist_type : uint8_t { code_hist_mark, code_hist_put, code_hist_del };
struct code_hist { enum code_hist_type type; char c; uint32_t pos; };

struct code
{
    struct ast *ast;

    struct { size_t len; struct code_str *list; } str;

    struct
    {
        bool skip;
        size_t it, top, cap;
        struct code_hist *list;
    } hist;

    bool modified;
    hash_val hash;
};


struct code *code_alloc(void)
{
    struct code *code = calloc(1, sizeof(*code));
    code->str.list = calloc(code_str_cap, sizeof(*code->str.list));
    code->ast = ast_alloc();
    return code;
}

void code_free(struct code *code)
{
    for (size_t i = 0; i < code->str.len; ++i)
        free(code->str.list[i].str);
    free(code->str.list);

    ast_free(code->ast);
    free(code);
}

bool code_empty(struct code *code)
{
    return !code->str.len;
}

size_t code_len(struct code *code)
{
    size_t len = 0;
    for (size_t i = 0; i < code->str.len; ++i)
        len += code->str.list[i].len;
    return len;
}

hash_val code_hash(struct code *code)
{
    if (code->hash) return code->hash;

    code->hash = hash_init();
    for (size_t i = 0; i < code->str.len; ++i) {
        const struct code_str *str = code->str.list + i;
        code->hash = hash_bytes(code->hash, str->str, str->len);
    }

    return code->hash;
}

size_t code_write(struct code *code, char *dst, size_t len)
{
    size_t sum = 0;
    const struct code_str *it = code->str.list;
    const struct code_str *end = it + code->str.len;

    for (; len && it < end; ++it) {
        size_t n = legion_min(len, it->len);
        memcpy(dst, it->str, n);
        dst += n; sum += n; len -= n;
    }

    return sum;
}


// -----------------------------------------------------------------------------
// hist
// -----------------------------------------------------------------------------

static void code_hist_push(
        struct code *code, enum code_hist_type type, char c, uint32_t pos)
{
    if (code->hist.skip) return;

    if (type == code_hist_mark) {
        if (!code->hist.top) return;
        if (code->hist.top > code->hist.it) return;
        if (code->hist.list[code->hist.top - 1].type == type) return;
    }

    if (code->hist.top == code->hist.cap) {
        size_t old = legion_xchg(&code->hist.cap,
                code->hist.cap ? code->hist.cap * 2 : 128);
        code->hist.list = realloc_zero(
                code->hist.list, old, code->hist.cap, sizeof(*code->hist.list));
    }

    struct code_hist *it = code->hist.list + code->hist.it;
    *it = (struct code_hist) { .type = type, .c = c, .pos = pos };
    code->hist.top = ++code->hist.it;
}

uint32_t code_undo(struct code *code)
{
    uint32_t pos = code_pos_nil;
    code->hist.skip = true;

    for (; code->hist.it; --code->hist.it) {
        struct code_hist *it = code->hist.list + (code->hist.it - 1);

        switch (it->type)
        {
        case code_hist_mark: { if (pos != code_pos_nil) { goto done; } break;}
        case code_hist_put: { code_delete(code, pos = it->pos); break; }
        case code_hist_del: { code_insert(code, pos = it->pos, it->c); pos++; break; }
        default: { assert(false); }
        }
    }

  done:
    code->hist.skip = false;
    return pos;
}

uint32_t code_redo(struct code *code)
{
    uint32_t pos = code_pos_nil;
    code->hist.skip = true;

    for (; code->hist.it < code->hist.top; ++code->hist.it) {
        struct code_hist *it = code->hist.list + code->hist.it;

        switch (it->type)
        {
        case code_hist_mark: { if (pos != code_pos_nil) { goto done; } break;}
        case code_hist_put: { code_insert(code, pos = it->pos, it->c); pos++; break; }
        case code_hist_del: { code_delete(code, pos = it->pos); break; }
        default: { assert(false); }
        }
    }

  done:
    code->hist.skip = false;
    return pos;
}


// -----------------------------------------------------------------------------
// updates
// -----------------------------------------------------------------------------

static struct code_str *code_prefix(struct code *code, struct code_str *it)
{
    // See code_check_update() for boundary condition avoidance.
    assert(code->str.len < code_str_cap);

    struct code_str *begin = code->str.list;
    struct code_str *end = begin + code->str.len;

    if (!it) it = begin;
    assert(it >= begin && it <= end);

    if (it < end) {
        memmove(it + 1, it, sizeof(*it) * (end - it));
        memset(it, 0, sizeof(*it));
    }

    code->str.len++;
    return it;
}

void code_reset(struct code *code)
{
    struct code_str *begin = code->str.list;
    struct code_str *end = begin + code->str.len;

    for (struct code_str *it = begin; it < end; ++it) free(it->str);
    memset(begin, 0, (end - begin) * sizeof(*begin));
    code->str.len = 0;

    code->hash = 0;
}

void code_set(struct code *code, const char *str, size_t len)
{
    code_reset(code);
    while (len && !str[len-1]) len--;

    struct code_str *it = code_prefix(code, NULL);
    if (!len) return;

    code_str_set(it, str, len);
    ast_parse(code->ast, it->str, it->len);
    it->ast.node = ast_begin(code->ast);
    it->ast.log = ast_log_begin(code->ast);

}

void code_update(struct code *code)
{
    if (!code->modified) return;
    code->modified = false;

    code->hash = 0;
    code_hist_push(code, code_hist_mark, 0, 0);

    size_t len = 0;
    for (size_t i = 0; i < code->str.len; ++i)
        len += code->str.list[i].len;

    struct code_str str = {0};
    for (size_t i = 0; i < code->str.len; ++i) {
        const struct code_str *it = code->str.list + i;
        code_str_append(&str, it->str, it->len);
    }

    ast_parse(code->ast, str.str, str.len);
    str.ast.node = ast_begin(code->ast);
    str.ast.log = ast_log_begin(code->ast);

    code_reset(code);
    code->str.list[code->str.len++] = str;
}

static void code_check_update(struct code *code)
{
    if (unlikely(code->str.len + 2 == code_str_cap))
        code_update(code);
}

static struct code_str *code_split(
        struct code *code, struct code_str *it, uint32_t *pos)
{
    assert(it->ast.node);

    struct code_str *next = code_prefix(code, it + 1);

    uint32_t ast = it->ast.node->pos;
    uint32_t ast_pos = ast + *pos;
    ast_it ast_it = ast_next(code->ast, it->ast.node, ast_pos);

    bool splits = ast_pos <= ast_it->pos + ast_it->len;

    uint32_t begin_ast = ast_it->pos + (splits ? 0 : ast_it->len);
    uint32_t end_ast = (ast_it + 1)->pos;

    uint32_t begin = begin_ast - ast;
    uint32_t end = end_ast - ast;

    code_str_set(next, it->str + end, it->len - end);
    next->ast.node = ast_it + 1;
    next->ast.log = ast_log_next(code->ast, next->ast.node->pos, it->ast.log);

    if (!begin) {
        it->len = end;
        it->ast.node = nullptr;
        it->ast.log = nullptr;
        return it;
    }

    next = code_prefix(code, next);
    code_str_set(next, it->str + begin, end - begin);
    it->len = begin;
    *pos -= begin;
    return next;
}

void code_insert(struct code *code, uint32_t pos, char c)
{
    code_check_update(code);
    code->modified = true;
    code->hash = 0;

    code_hist_push(code, code_hist_put, c, pos);

    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;
    for (; it < end && pos > it->len; ++it) pos -= it->len;

    if (pos == it->len && it->ast.node) {
        struct code_str *next = it + 1;
        if (next < end && !next->ast.node) {
            pos -= it->len;
            it++;
        }
    }

    assert (it < end);

    if (!it->ast.node) {
        code_str_put(it, pos, c);
        return;
    }

    if (pos == it->len) {
        code_str_put(code_prefix(code, it + 1), 0, c);
        return;
    }

    struct code_str *split = code_split(code, it, &pos);
    code_str_put(split, pos, c);
}

void code_delete(struct code *code, uint32_t pos)
{
    const size_t abs = pos;

    code_check_update(code);
    code->modified = true;
    code->hash = 0;

    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;
    for (; it < end && pos >= it->len; ++it) pos -= it->len;
    if (it == end) return;

    code_hist_push(code, code_hist_del, it->str[pos], abs);

    if (!it->ast.node) {
        code_str_del(it, pos);
        return;
    }

    if (pos == it->len - 1) {
        code_str_del(it, pos);
        return;
    }

    struct code_str *split = code_split(code, it, &pos);
    code_str_del(split, pos);
}


// -----------------------------------------------------------------------------
// navigation
// -----------------------------------------------------------------------------

struct code_it code_begin(struct code *code, uint32_t row)
{
    uint32_t pos = 0, rows = 0;
    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;

    for (; it < end && rows < row; ++it) {
        for (pos = 0; pos < it->len; ++pos) {
            if (unlikely(it->str[pos] == '\n') && ++rows == row) {
                ++pos; goto done;
            }
        }
    }
done:
    return (struct code_it) {
        .node = it, .pos = pos,
        .row = rows, .col = 0,
        .len = 0, .str = nullptr,
        .ast = { 0 },
    };
}

bool code_step(struct code *code, struct code_it *it)
{
    struct code_str *str = it->node;
    struct code_str *str_end = code->str.list + code->str.len;

    uint32_t old = it->pos;
    if (it->len) it->pos += it->len;
    if (it->ast.node && it->ast.node->pos == old) ++it->ast.node;
    it->str = nullptr;
    it->len = 0;

    it->ast.use_node = false;
    it->ast.use_log = false;

    struct rowcol rc = { .row = it->row, .col = it->col };
    void update_rc(void)
    {
        uint32_t len = !it->len ? str->len - old : it->pos - old;
        rc = rowcol_add(rc, rowcol(str->str + old, len));
        old = 0;
    }

    for (; str < str_end; ++str) {

        while (unlikely(it->pos < str->len) && str_is_space(str->str[it->pos]))
            it->pos++;

        if (unlikely(it->pos == str->len)) {
            update_rc();
            it->pos = 0;
            it->ast.node = nullptr;
            it->ast.log = nullptr;
            continue;
        }

        if (unlikely(!str->ast.node)) {
            while ( it->pos + it->len < str->len &&
                    !str_is_space(str->str[it->pos + it->len]))
                it->len++;
            update_rc();
            break;
        }

        uint32_t ast = str->ast.node->pos;

        ast_it ast_node = it->ast.node ? it->ast.node : str->ast.node;
        it->ast.node = ast_node = ast_next(code->ast, ast_node, it->pos + ast);

        ast_log_it ast_log = it->ast.log ? it->ast.log : str->ast.log;
        it->ast.log = ast_log_next(code->ast, it->pos + ast, ast_log);

        if (it->pos == ast_node->pos - ast) {
            it->len = legion_min(ast_node->len, str->len - it->pos);
            it->ast.use_node = true;
            update_rc();
            break;
        }

        if (ast_log && it->pos == ast_log->pos - ast) {
            it->len = legion_min(ast_log->len, str->len - it->pos);
            it->ast.use_log = true;
            update_rc();
            break;
        }

        if (it->pos + ast >= ast_node->pos + ast_node->len)
            ast_node++;

        uint32_t end = legion_min(str->len, ast_node->pos - ast);
        if (ast_log && it->pos < ast_log->pos - ast)
            end = legion_min(end, ast_log->pos - ast);

        while ( it->pos + it->len < end &&
                !str_is_space(str->str[it->pos + it->len]))
            it->len++;

        update_rc();
        break;
    }

    if (str == str_end) return false;

    assert(it->len);
    assert(it->pos + it->len <= str->len);

    it->node = str;
    it->str = str->str + it->pos;
    it->row = rc.row;
    it->col = rc.col;

    return true;
}


struct rowcol code_rowcol(const struct code *code)
{
    struct rowcol rc = { .row = 1, .col = 0 };
    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;

    uint32_t col = 0;
    for (; it < end; ++it) {
        for (uint32_t i = 0; i < it->len; ++i) {
            if (likely(it->str[i] != '\n')) { col++; continue; }
            rc.col = legion_max(rc.col, legion_xchg(&col, 0));
            rc.row++;
        }
    }
    rc.col = legion_max(rc.col, col);

    return rc;

}

struct rowcol code_rowcol_for(const struct code *code, uint32_t pos)
{
    struct rowcol rc = {0};
    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;

    for (; it < end && it->len < pos; pos -= it->len, it++)
        rc = rowcol_add(rc, rowcol(it->str, it->len));

    assert(pos <= it->len);
    return rowcol_add(rc, rowcol(it->str, legion_min(pos, it->len)));
}

uint32_t code_pos_for(const struct code *code, uint32_t row, uint32_t col)
{
    struct rowcol rc = {0};
    struct code_str *it = code->str.list;
    struct code_str *end = it + code->str.len;

    uint32_t pos = 0;
    for (; it < end; ++it) {
        for (uint32_t i = 0; i < it->len; ++i, ++pos) {
            if (likely(it->str[i] != '\n')) { rc.col++; continue; }
            if (likely(rc.row < row)) { rc.col = 0; rc.row++; continue; }
            return col >= rc.col ? pos : pos - (rc.col - col);
        }
    }

    return pos;
}

const struct code_str *code_inc_str(struct code *code)
{
    code_update(code);
    assert(code->str.len == 1);
    return code->str.list;
}

static char code_inc(const struct code_str *str, uint32_t *pos, int32_t inc)
{
    if (inc > 0) {
        if (*pos == str->len) return 0;
        ++(*pos);
    }
    if (inc < 0) {
        if (!*pos) return 0;
        --(*pos);
    }
    return str->str[(*pos)];
}


// I pity any who have to modify this hellish function
uint32_t code_move_row(struct code *code, uint32_t pos, int32_t inc)
{
    assert(inc);
    const struct code_str *str = code_inc_str(code);

    char c = 0;
    uint32_t ix = pos;
    uint32_t cols = 0;
    while ((c = code_inc(str, &ix, -1)) && c != '\n') cols++;

    if (inc < 0) {
        if (!(c = code_inc(str, &ix, -1)))
            return c == '\n' ? 0 : pos;

        c = str->str[ix];
        while (c != '\n' && (c = code_inc(str, &ix, -1)));
        if (c == '\n') code_inc(str, &ix, +1);
    }

    if (inc > 0) {
        c = str->str[ix = pos];
        while (c != '\n' && (c = code_inc(str, &ix, +1)));
        if (!code_inc(str, &ix, +1)) return ix;
    }

    c = str->str[ix];
    while (cols && c != '\n' && (c = code_inc(str, &ix, +1))) cols--;
    return ix;
}

uint32_t code_move_col(struct code *code, uint32_t pos, int32_t inc)
{
    assert(inc);
    const struct code_str *str = code_inc_str(code);

    code_inc(str, &pos, inc);
    return pos;
}

uint32_t code_move_home(struct code *code, uint32_t pos)
{
    const struct code_str *str = code_inc_str(code);

    char c = str->str[pos];
    if (c == '\n') c = code_inc(str, &pos, -1);
    while (c != '\n' && (c = code_inc(str, &pos, -1)));
    if (c == '\n') code_inc(str, &pos, +1);

    return pos;
}

uint32_t code_move_end(struct code *code, uint32_t pos)
{
    const struct code_str *str = code_inc_str(code);

    char c = str->str[pos];
    while (c != '\n' && (c = code_inc(str, &pos, +1)));
    return pos;
}


// -----------------------------------------------------------------------------
// dbg
// -----------------------------------------------------------------------------

void code_dump(struct code *code)
{
    ast_dump(code->ast); dbg("");

    ast_it ast_first = ast_begin(code->ast);
    ast_log_it log_first = ast_log_begin(code->ast);
    struct rowcol rc = code_rowcol(code);

    dbgf("code.str: len=%zu, rc={%u, %u}", code->str.len, rc.row, rc.col);
    for (size_t i = 0; i < code->str.len; ++i) {
        struct code_str *str = code->str.list + i;

        char buf[30] = {0};
        unsigned len = legion_min(sizeof(buf), str->len);
        memcpy(buf, str->str, len);
        for (size_t i = 0; i < len; ++i) if (buf[i] == '\n') buf[i] = '_';

        dbgf("  [%02zu] len=%04u/%04u, ast=%04lu:%04lu, str='%.*s'",
                i, str->len, str->cap,
                str->ast.node ? str->ast.node - ast_first : 0,
                str->ast.log ? str->ast.log - log_first : 0,
                len, buf);
    }

    dbg("code.it:");
    struct code_it it = code_begin(code, 0);
    for (size_t i = 0; code_step(code, &it); ++i) {

        char buf[30] = {0};
        unsigned len = legion_min(sizeof(buf), it.len);
        memcpy(buf, it.str, len);
        for (size_t i = 0; i < len; ++i) if (buf[i] == '\n') buf[i] = '_';

        dbgf("  [%04zu] node=%04lu, pos=%04u, rc=%03u:%03u, ast.node=%d:%04lu, ast.log=%d:%04lu, len=%04u str='%.*s'",
                i, it.node - code->str.list,
                it.pos, it.row, it.col,
                it.ast.use_node, it.ast.node ? it.ast.node - ast_first : 0,
                it.ast.use_log, it.ast.log ? it.ast.log - log_first : 0,
                len, len, buf);
    }

    /* dbgf("code.rows: rc={%u, %u}", rc.row, rc.col); */
    /* for (uint32_t row = 0; row < rc.row + 1u; ++row) { */
    /*     struct code_it it = code_begin(code, row); */
    /*     bool ok = code_step(code, &it); */
    /*     dbgf("  [%03u] ok=%d, node=%04lu, pos=%04u, rc=%03u:%03u, ast=%04lu:%04lu", */
    /*             row, ok, it.node - code->str.list, */
    /*             it.pos, it.row, it.col, */
    /*             it.ast.node - ast_first, it.ast.log - log_first); */
    /* } */
}
