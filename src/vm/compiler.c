/* vm_compiler.c
   Rémi Attab (remi.attab@gmail.com), 21 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/vm.h"
#include "utils/vec.h"
#include "utils/text.h"

#include <ctype.h>
#include <stdarg.h>


// -----------------------------------------------------------------------------
// compiler
// -----------------------------------------------------------------------------

struct compiler
{
    struct {
        struct text *text;
        struct line *line;
        size_t row, col;

        size_t tok;
        char token[128];
    } in;

    struct {
        uint8_t *base;
        size_t i, len;
    } out;

    struct {
        size_t len, cap;
        struct mod_err *list;
    } err;

    struct {
        struct htable is;
        struct htable wants;
    } lbl;

    struct {
        size_t len, cap;
        struct mod_index *list;
    } index;
};

static bool compiler_eof(struct compiler *comp)
{
    return comp->in.line == NULL;
}

static char compiler_peek(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    return comp->in.line->c[comp->in.col];
}

static char compiler_next(struct compiler *comp)
{
    if (compiler_eof(comp)) return 0;
    char c = compiler_peek(comp);
    if (!c) {
        comp->in.line = comp->in.line->next;
        comp->in.row++;
        comp->in.col = 0;
    } else comp->in.col++;
    return compiler_eof(comp) ? 0 : c;
}

static void compiler_skip(struct compiler *comp)
{
    if (compiler_eof(comp)) return;
    comp->in.line = comp->in.line->next;
    comp->in.col = 0;
    comp->in.row++;
}

static bool compiler_eol(struct compiler *comp)
{
    if (!compiler_peek(comp)) return true;

    while (!compiler_eof(comp)) {
        char c = compiler_next(comp);
        if (isblank(c)) continue;
        if (c == '#') { compiler_skip(comp); return true; }
        if (!c) return true;
        return false;
    }
    return true;
}

static const char *compiler_start(struct compiler *comp, size_t *len)
{
    char c = 0;
    while (!compiler_eof(comp)) {
        c = compiler_next(comp);
        if (c == '#') { compiler_skip(comp); continue; }
        if (isblank(c)) continue;
        if (!c) continue;
        break;
    }
    if (compiler_eof(comp)) return NULL;

    comp->in.tok = 0;
    while (!compiler_eof(comp)) {
        if (!c || c == '#' || isspace(c)) break;
        comp->in.token[comp->in.tok++] = c;

        if (!compiler_peek(comp)) break;
        c = compiler_next(comp);
    }

    comp->in.token[comp->in.tok++] = 0;
    *len = comp->in.tok;
    return comp->in.token;
}

static const char *compiler_arg(struct compiler *comp, size_t *len)
{
    char c = 0;
    while ((c = compiler_peek(comp))) {
        if (isblank(c)) { compiler_next(comp); continue; }
        if (c == 0 || c == '#') return NULL;
        break;
    }
    if (!c) return NULL;

    comp->in.tok = 0;
    while ((c = compiler_peek(comp))) {
        if (isspace(c) || c == '#') break;
        comp->in.token[comp->in.tok++] = compiler_next(comp);
    }

    comp->in.token[comp->in.tok++] = 0;
    *len = comp->in.tok;
    return comp->in.token;
}

static bool compiler_atom(const char *str, size_t len, uint64_t *val)
{
    assert(str[0] == '!');
    if (len - 1 > vm_atom_cap) return false;

    atom_t atom = {0};
    memcpy(atom, str + 1, len - 1);

    *val = vm_atom(&atom);
    return true;
}

static bool compiler_mod(const char *str, size_t len, mod_t *mod)
{
    assert(str[0] == '!');
    if (len - 1 > vm_atom_cap) return false;

    atom_t atom = {0};
    memcpy(atom, str + 1, len - 1);

    *mod = mods_find(&atom);
    return true;
}

static bool compiler_num(const char *str, size_t len, int64_t *val)
{
    *val = 0;
    if (len < 2) return false;

    if (str[0] == '0' && str[1] == 'x') {
        for (size_t i = 2; i < len; ++i) {
            char c = str[i];
            if (!c) break;
            *val <<= 4;

                 if (c >= '0' && c <= '9') *val |= c - '0';
            else if (c >= 'A' && c <= 'F') *val |= (c - 'A') + 10;
            else if (c >= 'a' && c <= 'f') *val |= (c - 'a') + 10;
            else if (c == ',') {}
            else return false;
        }
        return true;
    }

    bool neg = str[0] == '-';

    for (size_t i = neg ? 1 : 0; i < len-1; ++i) {
        char c = str[i];
        *val *= 10;
        if (c >= '0' && c <= '9') *val += c - '0';
        else if (c == ',') {}
        else return false;
    }

    if (neg) *val *= -1;
    return true;
}

static legion_printf(2, 3) void compiler_err(
        struct compiler *comp, const char *fmt, ...)
{
    if (comp->err.len == comp->err.cap) {
        size_t len = comp->err.cap ? comp->err.cap * 2 : 2;
        comp->err.list = realloc(comp->err.list, len * sizeof(*comp->err.list));
    }

    struct mod_err *err = &comp->err.list[comp->err.len];
    err->line = comp->in.row;

    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(err->str, mod_err_cap, fmt, args);
    va_end(args);


    comp->err.len++;
}

static void compiler_index(struct compiler *comp, size_t line, size_t byte)
{
    if (comp->index.len == comp->index.cap) {
        comp->index.cap = comp->index.cap ? comp->index.cap * 2 : 16;
        comp->index.list = realloc(comp->index.list, comp->index.cap * sizeof(*comp->index.list));
    }

    struct mod_index *index = &comp->index.list[comp->index.len];

    if (comp->index.len) {
        struct mod_index *prev = index-1;
        assert(prev->line < line && prev->byte < byte);
    }

    *index = (struct mod_index) { .line = line, .byte = byte };
    comp->index.len++;
}

static bool compiler_write_check(struct compiler *comp, size_t pos, size_t len)
{
    if (pos + len < comp->out.len) return true;
    compiler_err(comp, "mod too big");
    return false;
}

static void compiler_write(struct compiler *comp, uint8_t val)
{
    if (!compiler_write_check(comp, comp->out.i, sizeof(val))) return;
    comp->out.base[comp->out.i] = val;
    comp->out.i += sizeof(val);
}

static void compiler_write_ip_at(struct compiler *comp, size_t pos, ip_t val)
{
    if (!compiler_write_check(comp, pos, sizeof(val))) return;
    *((ip_t *) &comp->out.base[pos]) = val;
}

static void compiler_write_ip(struct compiler *comp, ip_t val)
{
    if (!compiler_write_check(comp, comp->out.i, sizeof(val))) return;
    *((ip_t *) &comp->out.base[comp->out.i]) = val;
    comp->out.i += sizeof(val);
}

static void compiler_write_word(struct compiler *comp, word_t val)
{
    if (!compiler_write_check(comp, comp->out.i, sizeof(val))) return;
    *((word_t *) &comp->out.base[comp->out.i]) = val;
    comp->out.i += sizeof(val);
}


static void compiler_label_def(
        struct compiler *comp, const char *label, size_t len)
{
    uint64_t hash = hash_str(label, len - 2); // remove :\0
    struct htable_ret ret = htable_put(&comp->lbl.is, hash, comp->out.i);
    if (!ret.ok) { compiler_err(comp, "undefined label: %s", label); return; }

    ret = htable_get(&comp->lbl.wants, hash);
    if (!ret.ok) return;

    struct vec64 *vec = (void *) ret.value;
    for (size_t i = 0; i < vec->len; ++i)
        compiler_write_ip_at(comp, make_ip(0, vec->vals[i]), comp->out.i);

    (void) htable_del(&comp->lbl.wants, hash);
    vec64_free(vec);
}

static void compiler_label_ref(
        struct compiler *comp, const char *label, size_t len)
{
    assert(label[0] == '@');

    uint64_t hash = hash_str(label + 1, len - 2); // remove @ and \0
    struct htable_ret ret = htable_get(&comp->lbl.is, hash);
    if (ret.ok) { compiler_write_ip(comp, make_ip(0, ret.value)); return; }

    ret = htable_get(&comp->lbl.wants, hash);

    struct vec64 *vec = (void *) ret.value;
    vec = vec64_append(vec, comp->out.i);

    ret = ret.ok ?
        htable_xchg(&comp->lbl.wants, hash, (uint64_t) vec) :
        htable_put(&comp->lbl.wants, hash, (uint64_t) vec);
    assert(ret.ok);

    compiler_write_ip(comp, 0);
}

static struct compiler *compiler_alloc(struct text *in)
{
    struct compiler *comp = calloc(1, sizeof(*comp));
    comp->in.text = in;
    comp->in.line = in->first;

    comp->out.len = 1 << 16;
    comp->out.base = calloc(1, comp->out.len);

    return comp;
}

static struct mod *compiler_output(struct compiler *comp)
{
    if (comp->out.i == comp->out.len)
        compiler_err(comp, "program too big");

    struct htable_bucket *bucket = htable_next(&comp->lbl.wants, NULL);
    for (; bucket; bucket = htable_next(&comp->lbl.wants, bucket)) {
        struct vec64 *vec = (void *) bucket->value;
        for (size_t i = 0; i < vec->len; ++i)
            compiler_err(comp, "missing label at '%lu'", vec->vals[i]);
    }

    return mod_alloc(
            comp->in.text,
            comp->out.base, comp->out.i,
            comp->err.list, comp->err.len,
            comp->index.list, comp->index.len);
}

static void compiler_free(struct compiler *comp)
{
    free(comp->index.list);
    free(comp->err.list);

    struct htable_bucket *bucket = htable_next(&comp->lbl.wants, NULL);
    for (; bucket; bucket = htable_next(&comp->lbl.wants, bucket))
        vec64_free((void *) bucket->value);

    htable_reset(&comp->lbl.wants);
    htable_reset(&comp->lbl.is);

    free(comp->out.base);
    free(comp);
}


// -----------------------------------------------------------------------------
// mod_compile
// -----------------------------------------------------------------------------

struct mod *mod_compile(struct text *source)
{
    assert(source);
    struct compiler *comp = compiler_alloc(source);

    size_t len = 0;
    const char *token = NULL;

    while (!compiler_eof(comp)) {
        token = compiler_start(comp, &len);
        if (!len || !token) break;

        if (token[len - 2] == ':') {
            if (unlikely(len > 32)) {
                compiler_err(comp, "label too long");
                compiler_skip(comp);
                continue;
            }
            compiler_label_def(comp, token, len);
            compiler_eol(comp);
            continue;
        }

        compiler_index(comp, comp->in.row, comp->out.i);

        struct op_spec *spec = op_spec(token, len - 1);
        if (!spec) {
            compiler_err(comp, "invalid opcode");
            compiler_skip(comp);
            continue;
        }
        compiler_write(comp, spec->op);

        switch (spec->arg) {
        case ARG_NIL: { break; }

        case ARG_LIT: {
            const char *lit = compiler_arg(comp, &len);
            if (!lit) { compiler_err(comp, "missing literal argument"); break; }

            if (lit[0] == '!') {
                uint64_t val = 0;
                if (!compiler_atom(lit, len, &val)) {
                    compiler_err(comp, "invalid atom");
                    break;
                }
                compiler_write_word(comp, val);
            }

            else {
                int64_t val = 0;
                if (!compiler_num(lit, len, &val)) {
                    compiler_err(comp, "invalid number");
                    break;
                }
                compiler_write_word(comp, val);
            }

            break;
        }

        case ARG_LEN: {
            const char *num = compiler_arg(comp, &len);
            if (!num) { compiler_err(comp, "missing length argument"); break; }

            int64_t val = 0;
            if (!compiler_num(num, len, &val)) { compiler_err(comp, "invalid length"); break; }
            if (val >= 0x7F) { compiler_err(comp, "length must be less then 128"); break; }
            compiler_write(comp, val);
            break;
        }

        case ARG_REG: {
            const char *reg = compiler_arg(comp, &len);
            if (!reg) { compiler_err(comp, "missing register argument"); break; }
            if (reg[0] != '$') { compiler_err(comp, "registers must start with $"); break; }
            if (len != 3 || reg[1] < '1' || reg[1] > '4') {
                compiler_err(comp, "unknown register");
                break;
            }
            compiler_write(comp, reg[1] - '1');
            break;
        }

        case ARG_OFF: {
            const char *label = compiler_arg(comp, &len);
            if (!label) { compiler_err(comp, "missing label argument"); break; }
            if (label[0] != '@') { compiler_err(comp, "labels must start with @"); break; }
            if (len > 32) { compiler_err(comp, "label too long"); break; }
            compiler_label_ref(comp, label, len);
            break;
        }

        case ARG_MOD: {
            const char *label = compiler_arg(comp, &len);
            if (!label) { compiler_err(comp, "missing dst argument"); break; }

            if (label[0] == '@') {
                if (len > 32) { compiler_err(comp, "label too long"); break; }
                compiler_label_ref(comp, label, len);
                break;
            }

            if (label[0] == '!') {
                mod_t mod = 0;
                if (!compiler_mod(label, len, &mod)) {
                    compiler_err(comp, "invalid mod");
                    break;
                }
                if (!mod) { compiler_err(comp, "unknown mod"); break; }
                compiler_write_ip(comp, make_ip(mod, 0));
                break;
            }

            compiler_err(comp, "CALL requires a mod or an atom");
            break;
        }

        default: assert(false && "unknown arg type");

        }

        if (!compiler_eol(comp)) {
            compiler_err(comp, "invalid extra arguments");
            compiler_skip(comp);
        }
    }

    compiler_index(comp, comp->in.row, comp->out.i);
    compiler_write(comp, OP_RESET);

    struct mod *mod = compiler_output(comp);
    compiler_free(comp);
    return mod;
}
