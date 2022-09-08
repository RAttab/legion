/* lisp_fn.c
   Rémi Attab (remi.attab@gmail.com), 15 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in lisp.c


// -----------------------------------------------------------------------------
// declarations
// -----------------------------------------------------------------------------

static void lisp_call(struct lisp *lisp);
static void lisp_call_mod(struct lisp *lisp, word mod);


// -----------------------------------------------------------------------------
// stmt
// -----------------------------------------------------------------------------

static bool lisp_stmt(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);
    switch (token->type)
    {

    case token_nil: {
        if (lisp->depth) lisp_err(lisp, "premature eof");
        return false;
    }

    case token_close: {
        if (!lisp->depth) lisp_err(lisp, "unexpected close token");
        else lisp->depth--;
        return false;
    }

    case token_open: {
        lisp->depth++;

        struct token *token = lisp_expect(lisp, token_symbol);
        if (!token) {
            lisp_goto_close(lisp);
            return false;
        }

        lisp_index_at(lisp, token);

        struct htable_ret ret = {0};
        uint64_t key = symbol_hash(&token->value.s);

        if ((ret = htable_get(&lisp->symb.fn, key)).ok)
            ((lisp_fn) ret.value)(lisp);
        else lisp_call(lisp);

        return true;
    }

    case token_number: {
        lisp_index(lisp);
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, token->value.w);
        return true;
    }

    case token_atom: {
        word value = atoms_get(lisp->atoms, &token->value.s);
        if (!value) {
            lisp_err(lisp, "unregistered atom '%s'", token->value.s.c);
            return true;
        }

        lisp_index(lisp);
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, value);
        return true;
    }

    case token_atom_make: {
        lisp_index(lisp);
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, atoms_make(lisp->atoms, &token->value.s));
        return true;
    }

    case token_reg: {
        lisp_index(lisp);
        lisp_write_op(lisp, OP_PUSHR);
        lisp_write_value(lisp, (uint8_t) token->value.w);
        return true;
    }

    case token_symbol: {
        lisp_index(lisp);
        if (lisp_is_reg(lisp, &token->value.s)) {
            lisp_write_op(lisp, OP_PUSHR);
            lisp_write_value(lisp, lisp_reg(lisp, &token->value.s));
        }
        else {
            lisp_write_op(lisp, OP_PUSH);
            lisp_write_value(lisp, lisp_const(lisp, &token->value.s));
        }
        return true;
    }

    default: { assert(false); }
    }
}

static void lisp_stmts(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) {
        // ensures that all statement return exactly one value on the stack.
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, (word) 0);
        return;
    }

    // If we don't use the return of the previous statment we need to pop it out
    // of the stack. Since we can't peek the next token, the simplest solution
    // is to SWAP & POP after the statement executes but that leaves garbage on
    // the stack which is problematic for our small vms.
    //
    // Instead we introduce a NOOP after every statement and overwrite it with a
    // POP if we need to discard the return value.
    while (true) {
        ip noop = lisp_ip(lisp);
        lisp_write_op(lisp, OP_NOOP);

        if (!lisp_stmt(lisp)) break;

        lisp_write_value_at(lisp, noop, (enum op_code) OP_POP);
    }
}


// -----------------------------------------------------------------------------
// defun/call
// -----------------------------------------------------------------------------

// Parses the static module syntax (used by call) as opposed to lisp_fn_mod
// which parses the dynamic module syntax (used by load). The version is omited
// for now as it makes the syntax more complicated and it's not clear whether
// it's needed or not.
//
// syntax:
//
//   local: symbol
//          label
//
//   mod:   (symbol [symbol])
//           mod     pub
//
static word lisp_parse_call(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);

    if (likely(token->type == token_symbol) || token->type == token_number)
        return 0;

    if (!lisp_assert_token(lisp, token, token_open))
        return -1;

    const struct mod *mod = NULL;
    {
        token = lisp_expect(lisp, token_symbol);
        if (!token) { lisp_goto_close(lisp); return -1; }

        mod_maj mod_maj = mods_find(lisp->mods, &token->value.s);
        if (!mod_maj) {
            lisp_err(lisp, "unknown mod: %s", token->value.s.c);
            lisp_goto_close(lisp);
            return -1;
        }

        mod = mods_latest(lisp->mods, mod_maj);
        if (!mod) {
            lisp_err(lisp, "unknown mod: %s (%x)", token->value.s.c, mod_maj);
            lisp_goto_close(lisp);
            return -1;
        }
    }

    token = lisp_next(lisp);

    ip ip = 0;
    if (token->type == token_symbol) {
        ip = mod_pub(mod, symbol_hash(&token->value.s));
        if (ip == MOD_PUB_UNKNOWN) {
            lisp_err(lisp, "unknown public symbol in mod: %s in %x",
                    token->value.s.c, mod->id);
            lisp_goto_close(lisp);
            return -1;
        }

        token = lisp_next(lisp);
    }

    if (!lisp_assert_token(lisp, token, token_close)) {
        lisp_goto_close(lisp);
        return -1;
    }

    return vm_pack(mod->id, ip);
}

static void lisp_call_mod(struct lisp *lisp, word mod)
{
    struct token token = lisp->token;

    reg args = 0;
    while (lisp_stmt(lisp)) ++args;
    if (args > 4) lisp_err(lisp, "too many arguments: %u > 4", (unsigned) args);
    lisp_index_at(lisp, &token);

    reg regs = 0;
    for (; regs < 4 && lisp->symb.regs[regs]; regs++);

    reg common = legion_min(regs, args);
    for (reg r = 0; r < common; ++r) {
        lisp_write_op(lisp, OP_ARG0 + r);
        lisp_write_value(lisp, (reg) (args - r - 1));
    }

    if (args > regs) {
        for (reg r = 0; r < (args - common); ++r) {
            lisp_write_op(lisp, OP_POPR);
            lisp_write_value(lisp, (reg) (args - r - 1));
        }
    }
    else if (regs > args) {
        for (reg r = common; r < regs; ++r) {
            lisp_write_op(lisp, OP_PUSHR);
            lisp_write_value(lisp, r);
        }
    }

    lisp_write_op(lisp, OP_CALL);

    if (mod) lisp_write_value(lisp, mod);
    else {
        // little-endian flips the byte ordering... fuck me...
        lisp_write_value(lisp, lisp_jmp(lisp, &token));
        lisp_write_value(lisp, (mod_id) 0);
    }

    for (reg i = 0; i < regs; ++i) {
        lisp_write_op(lisp, OP_SWAP); // ret value is on top of the stack
        lisp_write_op(lisp, OP_POPR);
        lisp_write_value(lisp, (reg) (regs - i - 1));
    }
}

static void lisp_call(struct lisp *lisp)
{
    lisp_call_mod(lisp, 0);
}

static void lisp_fn_call(struct lisp *lisp)
{
    word target = lisp_parse_call(lisp);
    if (target == -1) { lisp_goto_close(lisp); return; }

    lisp_call_mod(lisp, target);
}

static void lisp_fn_defun(struct lisp *lisp)
{
    struct token index = lisp->token;
    if (lisp->depth > 1) lisp_err(lisp, "nested function");

    // since we can have top level instructions, we don't want to actually
    // execute defun unless it's called so we instead jump over it.
    lisp_write_op(lisp, OP_JMP);
    ip skip = lisp_skip(lisp, sizeof(ip));

    {
        struct token *token = lisp_expect(lisp, token_symbol);
        if (!token) { lisp_goto_close(lisp); return; }
        lisp_label(lisp, &token->value.s);
    }

    if (!lisp_expect(lisp, token_open)) { lisp_goto_close(lisp); return; }

    struct token *token = NULL;
    for (reg reg = 0; (token = lisp_next(lisp))->type != token_close; ++reg) {
        if (!lisp_assert_token(lisp, token, token_symbol)) {
            lisp_goto_close(lisp);
            break;
        }

        if (reg < 4) lisp->symb.regs[reg] = symbol_hash(&token->value.s);
        else lisp_err(lisp, "too many parameters: %u > 4", reg + 1);
    }

    lisp_stmts(lisp);

    for (reg reg = 0; reg < 4; ++reg) lisp->symb.regs[reg] = 0;
    lisp_index_at(lisp, &index);

    // our return value is above the return ip on the stack. Swaping the two
    // ensures that we can call RET and that the return value will be on top of
    // the stack after we return. MAGIC!
    lisp_write_op(lisp, OP_SWAP);

    lisp_write_op(lisp, OP_RET);
    lisp_write_value_at(lisp, skip, lisp_ip(lisp));

    // every statement must return a value on the stack even if it's at the
    // top-level.
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);
}

static void lisp_fn_load(struct lisp *lisp)
{
    struct token index = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing load mod argument"); return; }

    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_LOAD);
    lisp_expect_close(lisp);
}


// -----------------------------------------------------------------------------
// builtins
// -----------------------------------------------------------------------------

// pseudo function that essentially returns whatever is at the top of the stack.
// This is mostly useful when dealing with statements that can return multiple
// values.
static void lisp_fn_head(struct lisp *lisp) { lisp_expect_close(lisp); }

// when writting assembly, we won't follow the rule where every statement must
// return one value on the stack which means that if used in a typical stmts
// block then things will get poped out of the stack arbitrarily.
//
// the asm statement essentially applies all the included statement as is with
// zero stack modification in between. It's still expected that the overall
// block pushes one or more element on the stack in used within lisp statements
// but that's up to the user to manage.
static void lisp_fn_asm(struct lisp *lisp)
{
    while (lisp_stmt(lisp));
}

static void lisp_fn_progn(struct lisp *lisp)
{
    lisp_stmts(lisp);
}

static void lisp_fn_defconst(struct lisp *lisp)
{
    if (lisp->depth > 1) lisp_err(lisp, "nested defconst");

    if (!lisp_expect(lisp, token_symbol)) { lisp_goto_close(lisp); return; }
    struct token token = lisp->token;

    word value = lisp_eval(lisp);

    uint64_t key = symbol_hash(&token.value.s);
    struct htable_ret ret = htable_put(&lisp->consts, key, value);
    if (!ret.ok)
        lisp_err(lisp, "redefinition of constant: %s", token.value.s.c);

    lisp_expect_close(lisp);

    // every statement must return a value on the stack even if it's at the
    // top-level.
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, value);
}

static void lisp_fn_let(struct lisp *lisp)
{
    struct token *token = lisp_expect(lisp, token_open);

    uint64_t regs[4] = {0};

    while ((token = lisp_next(lisp))) {
        if (token->type == token_close) break;
        if (token->type != token_open) {
            lisp_err(lisp, "unexpected token in argument list: %s != %s",
                    token_type_str(token->type), token_type_str(token_open));
            lisp_goto_close(lisp);
            break;
        }
        struct token index_reg = *token;

        uint64_t key = symbol_hash(&lisp_expect(lisp, token_symbol)->value.s);

        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing statement");
            continue;
        }

        lisp_index_at(lisp, &index_reg);

        reg reg = lisp_reg_alloc(lisp, key);
        lisp_write_op(lisp, OP_POPR);
        lisp_write_value(lisp, reg);
        regs[reg] = key;

        if (!lisp_expect(lisp, token_close))
            lisp_goto_close(lisp);
    }

    lisp_stmts(lisp);

    for (reg reg = 0; reg < array_len(regs); ++reg)
        lisp_reg_free(lisp, reg, regs[reg]);
}

static void lisp_fn_if(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) { // predicate
        lisp_err(lisp, "missing predicate-clause");
        return;
    }

    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_JZ);
    ip jmp_else = lisp_skip(lisp, sizeof(ip));

    if (!lisp_stmt(lisp)) { // true-clause
        lisp_err(lisp, "missing true-clause");
        return;
    }

    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_JMP);
    ip jmp_end = lisp_skip(lisp, sizeof(ip));
    lisp_write_value_at(lisp, jmp_else, lisp_ip(lisp));

    if (!lisp_stmt(lisp)) { // else-clause (optional)
        // if we have no else clause and we're not executing the if clause then
        // this ensures that we always return a value on the stack.
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, (word) 0);
    }
    else lisp_expect_close(lisp);

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_case(struct lisp *lisp)
{
    if (!lisp_stmt(lisp)) {
        lisp_err(lisp, "missing value-clause");
        lisp_goto_close(lisp);
        return;
    }

    if (!lisp_expect(lisp, token_open)) {
        lisp_goto_close(lisp);
        return;
    }

    size_t len = 0;
    ip jmp[32] = {0};
    struct token *token = NULL;

    while ((token = lisp_next(lisp))->type != token_close) {
        if (len == array_len(jmp)) {
            lisp_err(lisp, "too many case clauses: %zu >= %zu", len, array_len(jmp));
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        if (!lisp_assert_token(lisp, token, token_open)) {
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }
        lisp->depth++;

        lisp_write_op(lisp, OP_DUPE);

        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing value for case-clause");
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        lisp_write_op(lisp, OP_EQ);
        lisp_write_op(lisp, OP_JZ);
        ip next = lisp_skip(lisp, sizeof(ip));

        // Remove the case value
        lisp_write_op(lisp, OP_POP);

        lisp_stmts(lisp);

        lisp_write_op(lisp, OP_JMP);
        jmp[len] = lisp_skip(lisp, sizeof(ip));
        len++;

        lisp_write_value_at(lisp, next, lisp_ip(lisp));
    }

    // Optional default clause. If the default clause is not present then our
    // return value is the case value.
    if (!lisp_peek_close(lisp)) {
        if (!lisp_expect(lisp, token_open)) {
            lisp_goto_close(lisp);
            return;
        }
        lisp->depth++;

        token = lisp_expect(lisp, token_symbol);
        if (!token) {
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        uint64_t key = symbol_hash(&token->value.s);
        reg reg = lisp_reg_alloc(lisp, key);
        lisp_index_at(lisp, token);

        // pop case value into our register
        lisp_write_op(lisp, OP_POPR);
        lisp_write_value(lisp, reg);

        lisp_stmts(lisp);

        lisp_reg_free(lisp, reg, key);
    }

    for (size_t i = 0; i < len; ++i)
        lisp_write_value_at(lisp, jmp[i], lisp_ip(lisp));

    lisp_expect_close(lisp);
}

static void lisp_fn_when(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) { // predicate
        lisp_err(lisp, "missing predicate-clause");
        return;
    }
    lisp_index_at(lisp, &index);

    lisp_write_op(lisp, OP_JZ);
    ip jmp_else = lisp_skip(lisp, sizeof(ip));

    lisp_stmts(lisp);
    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_JMP);
    ip jmp_end = lisp_skip(lisp, sizeof(ip));

    lisp_write_value_at(lisp, jmp_else, lisp_ip(lisp));
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_unless(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) { // predicate
        lisp_err(lisp, "missing predicate-clause");
        return;
    }
    lisp_index_at(lisp, &index);

    lisp_write_op(lisp, OP_JNZ);
    ip jmp_else = lisp_skip(lisp, sizeof(ip));

    lisp_stmts(lisp);
    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_JMP);
    ip jmp_end = lisp_skip(lisp, sizeof(ip));

    lisp_write_value_at(lisp, jmp_else, lisp_ip(lisp));
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_while(struct lisp *lisp)
{
    // init
    {
        // used to ensure that we always return one value on the stack
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, (word) 0);
    }

    // predicate
    ip jmp_end = 0;
    ip jmp_pred = lisp_ip(lisp);
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing predicate-clause");
            return;
        }

        lisp_write_op(lisp, OP_JZ);
        jmp_end = lisp_skip(lisp, sizeof(ip));
    }

    // stmts
    {
        // we will always have a value on the stack before executing the stmt
        // block (see push at the start) so get rid of it.
        lisp_write_op(lisp, OP_POP);

        lisp_stmts(lisp); // loop-clause

        lisp_write_op(lisp, OP_JMP);
        lisp_write_value(lisp, jmp_pred);
    }

    lisp_write_value_at(lisp, jmp_end, lisp_ip(lisp));
}

static void lisp_fn_for(struct lisp *lisp)
{
    reg reg = 0;
    uint64_t key = 0;

    // init
    {
        if (!lisp_expect(lisp, token_open)) {
            lisp_err(lisp, "missing init-clause");
            lisp_goto_close(lisp);
            return;
        }

        struct token *token = lisp_expect(lisp, token_symbol);
        if (!token) {
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        key = symbol_hash(&token->value.s);
        reg = lisp_reg_alloc(lisp, key);

        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing init-clause initializer statement");
            lisp_goto_close(lisp);
            lisp_goto_close(lisp);
            return;
        }

        lisp_write_op(lisp, OP_POPR);
        lisp_write_value(lisp, reg);

        if (!lisp_expect(lisp, token_close))
            lisp_goto_close(lisp);

        // used to ensure that we always return one value on the stack
        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, (word) 0);
    }

    // predicate
    ip jmp_true = 0;
    ip jmp_false = 0;
    ip jmp_pred = lisp_ip(lisp);
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing predicate-clause");
            lisp_goto_close(lisp);
            return;
        }

        lisp_write_op(lisp, OP_JNZ);
        jmp_true = lisp_skip(lisp, sizeof(ip));

        lisp_write_op(lisp, OP_JMP);
        jmp_false = lisp_skip(lisp, sizeof(ip));
    }

    // inc
    ip jmp_inc = lisp_ip(lisp);
    {
        if (!lisp_stmt(lisp)) {
            lisp_err(lisp, "missing increment-clause");
            lisp_goto_close(lisp);
            return;
        }

        lisp_write_op(lisp, OP_POPR);
        lisp_write_value(lisp, reg);

        lisp_write_op(lisp, OP_JMP);
        lisp_write_value(lisp, jmp_pred);
    }

    // stmts
    lisp_write_value_at(lisp, jmp_true, lisp_ip(lisp));
    {
        // we will always have a value on the stack before executing the stmt
        // block (see push at the start) so get rid of it.
        lisp_write_op(lisp, OP_POP);

        lisp_stmts(lisp); // loop-clause

        lisp_write_op(lisp, OP_JMP);
        lisp_write_value(lisp, jmp_inc);
    }

    // end
    lisp_write_value_at(lisp, jmp_false, lisp_ip(lisp));
    lisp_reg_free(lisp, reg, key);
}

static void lisp_fn_set(struct lisp *lisp)
{
    struct token index = lisp->token;

    struct token *token = lisp_expect(lisp, token_symbol);
    if (!token) { lisp_goto_close(lisp); return; }
    reg reg = lisp_reg(lisp, &token->value.s);

    if (!lisp_stmt(lisp)) {
        lisp_err(lisp, "missing set value");
        return;
    }

    lisp_index_at(lisp, &index);

    lisp_write_op(lisp, OP_POPR);
    lisp_write_value(lisp, reg);

    lisp_write_op(lisp, OP_PUSHR);
    lisp_write_value(lisp, reg);

    lisp_expect_close(lisp);
}

static void lisp_fn_id(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) { // type
        lisp_err(lisp, "missing type argument");
        return;
    }

    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) id_shift);
    lisp_write_op(lisp, OP_BSL);

    if (!lisp_stmt(lisp)) { // index
        lisp_err(lisp, "missing index argument");
        return;
    }

    lisp_index_at(lisp, &index);
    lisp_write_op(lisp, OP_ADD);

    lisp_expect_close(lisp);
}

static void lisp_fn_self(struct lisp *lisp)
{
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);
    lisp_expect_close(lisp);
}

// Keep in sync with lisp_eval_mod
static void lisp_fn_mod(struct lisp *lisp)
{
    struct token *token = lisp_next(lisp);

    // self-referencial mod
    if (token->type == token_close) {
        const struct mod *mod = mods_latest(lisp->mods, lisp->mod_maj);
        assert(mod);
        mod_id id = make_mod(lisp->mod_maj, mod_version(mod->id) + 1);

        lisp_write_op(lisp, OP_PUSH);
        lisp_write_value(lisp, (word) id);

        lisp_assert_close(lisp, token);
        return;
    }

    if (!lisp_assert_token(lisp, token, token_symbol)) {
        lisp_goto_close(lisp);
        return;
    }

    mod_maj mod_maj = mods_find(lisp->mods, &token->value.s);
    if (!mod_maj) {
        lisp_err(lisp, "unknown mod: %s", token->value.s.c);
        lisp_goto_close(lisp);
        return;
    }

    mod_id id = 0;
    if ((token = lisp_next(lisp))->type == token_number) {
        id = make_mod(mod_maj, token->value.w);
        token = lisp_next(lisp);
    }
    else {
        const struct mod *mod = mods_latest(lisp->mods, mod_maj);
        assert(mod);
        id = mod->id;
    }

    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) id);

    lisp_assert_close(lisp, token);
}

static void lisp_fn_io(struct lisp *lisp)
{
    struct token token = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing op argument"); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing dst argument"); return; }

    lisp_index_at(lisp, &token);
    lisp_write_op(lisp, OP_SWAP);
    lisp_write_op(lisp, OP_PACK);

    size_t len = 1;
    while (lisp_stmt(lisp)) len++;

    if (len > vm_io_cap)
        lisp_err(lisp, "too many io arguments: %zu > %u", len, vm_io_cap);

    lisp_index_at(lisp, &token);
    lisp_write_op(lisp, OP_IO);
    lisp_write_value(lisp, (uint8_t) len);
}

// shorthand for the very common pattern:
//   (progn (io <op> <args>) (head))
static void lisp_fn_ior(struct lisp *lisp)
{
    lisp_fn_io(lisp);
    lisp_write_op(lisp, OP_POP);
}

static void lisp_fn_sub(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) { lisp_err(lisp, "missing first argument"); return; }

    if (!lisp_stmt(lisp)) {
        lisp_index_at(lisp, &index);
        lisp_write_op(lisp, OP_NEG);
        return;
    }

    do {
        lisp_index_at(lisp, &index);
        lisp_write_op(lisp, OP_SUB);
    } while (lisp_stmt(lisp));
}


static void lisp_fn_yield(struct lisp *lisp)
{
    lisp_write_op(lisp, OP_YIELD);

    // all statement must return a value on the stack.
    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);

    lisp_expect_close(lisp);
}

static void lisp_fn_assert(struct lisp *lisp)
{
    struct token index = lisp->token;

    if (!lisp_stmt(lisp)) {
        lisp_err(lisp, "missing predicate");
        return;
    }

    lisp_index_at(lisp, &index);

    lisp_write_op(lisp, OP_JNZ);
    ip jmp_false = lisp_skip(lisp, sizeof(jmp_false));

    lisp_write_op(lisp, OP_FAULT);
    lisp_write_value_at(lisp, jmp_false, lisp_ip(lisp));

    lisp_write_op(lisp, OP_PUSH);
    lisp_write_value(lisp, (word) 0);

    lisp_expect_close(lisp);
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

static void lisp_fn_0(struct lisp *lisp, enum op_code op, const char *str)
{
    (void) str;
    lisp_write_value(lisp, op);
    lisp_expect_close(lisp);
}

static void lisp_fn_1(struct lisp *lisp, enum op_code op, const char *str)
{
    struct token index = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing argument", str);  return; }

    lisp_index_at(lisp, &index);
    lisp_write_value(lisp, op);
    lisp_expect_close(lisp);
}

static void lisp_fn_2(struct lisp *lisp, enum op_code op, const char *str)
{
    struct token index = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing first argument", str); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing second argument", str); return; }

    lisp_index_at(lisp, &index);
    lisp_write_value(lisp, op);
    lisp_expect_close(lisp);
}

static void lisp_fn_n(struct lisp *lisp, enum op_code op, const char *str)
{
    struct token index = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing first argument", str); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing second argument", str); return; }

    do {
        lisp_index_at(lisp, &index);
        lisp_write_value(lisp, op);
    } while (lisp_stmt(lisp));
}

static void lisp_fn_compare(struct lisp *lisp, enum op_code op, const char *str)
{
    struct token index = lisp->token;
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing first argument", str); return; }
    if (!lisp_stmt(lisp)) { lisp_err(lisp, "%s: missing second argument", str); return; }

    lisp_index_at(lisp, &index);

    // gotta swap before the compare as the lisp semantics differ from the vm
    // semantics
    lisp_write_op(lisp, OP_SWAP);
    lisp_write_op(lisp, op);

    lisp_expect_close(lisp);
}

#define define_fn_ops(fn, op, arg)                      \
    static void lisp_fn_ ## fn(struct lisp *lisp)       \
    {                                                   \
        lisp_fn_ ## arg(lisp, OP_ ## op, #op);          \
    }

    define_fn_ops(not, NOT, 1)
    define_fn_ops(and, AND, n)
    define_fn_ops(xor, XOR, n)
    define_fn_ops(or, OR, n)
    define_fn_ops(bnot, BNOT, 1)
    define_fn_ops(band, BAND, n)
    define_fn_ops(bxor, BXOR, n)
    define_fn_ops(bor, BOR, n)
    define_fn_ops(bsl, BSL, 2)
    define_fn_ops(bsr, BSR, 2)

    define_fn_ops(add, ADD, n)
    // SUB -> lisp_fn_sub
    define_fn_ops(mul, MUL, n)
    define_fn_ops(lmul, LMUL, 2)
    define_fn_ops(div, DIV, 2)
    define_fn_ops(rem, REM, 2)

    define_fn_ops(eq, EQ, compare)
    define_fn_ops(ne, NE, compare)
    define_fn_ops(gt, GT, compare)
    define_fn_ops(ge, GE, compare)
    define_fn_ops(lt, LT, compare)
    define_fn_ops(le, LE, compare)
    define_fn_ops(cmp, CMP, compare)

    define_fn_ops(reset, RESET, 0)
    define_fn_ops(tsc, TSC, 0)
    define_fn_ops(fault, FAULT, 0)

    define_fn_ops(pack, PACK, 2)
    define_fn_ops(unpack, UNPACK, 1)

#undef define_fn_ops


// -----------------------------------------------------------------------------
// index
// -----------------------------------------------------------------------------

static void lisp_fn_register(void)
{
#define register_fn(fn)                                                 \
    do {                                                                \
        struct symbol symbol = make_symbol_len(#fn, sizeof(#fn));       \
        lisp_register_fn(symbol_hash(&symbol), lisp_fn_ ## fn);         \
    } while (false)

#define register_fn_str(fn, str)                                        \
    do {                                                                \
        struct symbol symbol = make_symbol(str);                        \
        lisp_register_fn(symbol_hash(&symbol), lisp_fn_ ## fn);         \
    } while (false)

    register_fn(defun);
    register_fn(call);
    register_fn(load);

    register_fn(head);
    register_fn(asm);
    register_fn(progn);
    register_fn(defconst);
    register_fn(let);
    register_fn(if);
    register_fn(case);
    register_fn(when);
    register_fn(unless);
    register_fn(while);
    register_fn(for);

    register_fn(set);
    register_fn(id);
    register_fn(self);
    register_fn(mod);
    register_fn(io);
    register_fn(ior);

    register_fn(not);
    register_fn(and);
    register_fn(xor);
    register_fn(or);
    register_fn(bnot);
    register_fn(band);
    register_fn(bxor);
    register_fn(bor);
    register_fn(bsl);
    register_fn(bsr);

    register_fn_str(add, "+");
    register_fn_str(sub, "-");
    register_fn_str(mul, "*");
    register_fn(lmul);
    register_fn_str(div, "/");
    register_fn(rem);

    register_fn_str(eq, "=");
    register_fn_str(ne, "/=");
    register_fn_str(gt, ">");
    register_fn_str(ge, ">=");
    register_fn_str(lt, "<");
    register_fn_str(le, "<=");
    register_fn(cmp);

    register_fn(reset);
    register_fn(yield);
    register_fn(tsc);
    register_fn(fault);
    register_fn(assert);

    register_fn(pack);
    register_fn(unpack);

#undef register_fn_str
#undef register_fn
}
