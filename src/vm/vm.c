/* vm.c
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// flags
// -----------------------------------------------------------------------------

static uint8_t flag_faults =
    FLAG_FAULT_USER |
    FLAG_FAULT_REG |
    FLAG_FAULT_STACK |
    FLAG_FAULT_CODE |
    FLAG_FAULT_MATH |
    FLAG_FAULT_IO;


// -----------------------------------------------------------------------------
// vm
// -----------------------------------------------------------------------------

struct vm *vm_alloc(uint8_t stack, uint8_t speed)
{
    struct vm *vm = mem_align_alloc(vm_len(stack), sys_cache_line_len);
    vm_init(vm, stack, speed);
    return vm;
}

void vm_free(struct vm *vm)
{
    mem_free(vm);
}

void vm_init(struct vm *vm, uint8_t stack, uint8_t speed)
{
    vm->specs.stack = vm_stack_len(stack);
    vm->specs.speed = vm_speed(speed);
}

void vm_suspend(struct vm *vm)
{
    vm->flags |= FLAG_SUSPENDED;
}

void vm_resume(struct vm *vm)
{
    vm->flags &= ~FLAG_SUSPENDED;
}

bool vm_fault(struct vm *vm)
{
    return vm->flags & flag_faults;
}

void vm_io_fault(struct vm *vm)
{
    vm->flags |= FLAG_FAULT_IO;
}

void vm_push(struct vm *vm, vm_word word)
{
    if (unlikely(vm->sp == vm->specs.stack)) {
        vm->flags |= FLAG_FAULT_STACK;
        return;
    }
    vm->stack[vm->sp++] = word;
}

size_t vm_io_read(struct vm *vm, vm_word *dst)
{
    vm->flags &= ~FLAG_IO;
    if (vm->io > vm_io_cap) { vm_io_fault(vm); return 0; }

    size_t len = legion_min(vm->io, vm->sp);

    vm->sp -= len;
    memcpy(dst, vm->stack + vm->sp, len * sizeof(*dst));

    return len;
}

void vm_reset(struct vm *vm)
{
    uint8_t stack = vm->specs.stack;
    uint8_t speed = vm->specs.speed;

    memset(vm, 0, sizeof(*vm) + sizeof(vm->stack[0]) * vm->specs.stack);

    vm->specs.stack = stack;
    vm->specs.speed = speed;
    vm->ip = 0;
}

size_t vm_dbg(struct vm *vm, char *dst, size_t len)
{
    size_t orig = len;

    size_t n = 0;
    n = snprintf(dst, len, "spec:  { stack:%u, speed:%u }\n",
            (unsigned) vm->specs.stack, (unsigned) vm->specs.speed);
    dst += n; len -= n;

    n = snprintf(dst, len, "gen:   { ip=%08x, sp=%02x, sbp=%02x, flags:%02x, io:%02x }, tsc:%08x }\n",
            vm->ip,
            (unsigned) vm->sp, (unsigned) vm->sbp,
            (unsigned) vm->flags, (unsigned) vm->io,
            vm->tsc);
    dst += n; len -= n;

    n = snprintf(dst, len, "reg:   [ 0:%016lx 1:%016lx 2:%016lx 3:%016lx ]\n",
            vm->regs[0], vm->regs[1], vm->regs[2], vm->regs[3]);
    dst += n; len -= n;

    n = snprintf(dst, len, "stack: [ ");
    dst += n; len -= n;

    for (size_t i = 0; i < vm->sp; ++i) {
        n = snprintf(dst, len, "%zu:%016lx ", i, vm->stack[i]);
        dst += n; len -= n;
    }

    n = snprintf(dst, len, "]\n");
    dst += n; len -= n;

    return orig - len;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------


mod_id vm_exec(struct vm *vm, const struct mod *mod)
{
    if (unlikely(vm_fault(vm))) return VM_FAULT;
    if (unlikely(vm->flags & FLAG_SUSPENDED)) return 0;

    if (unlikely(vm->ip >= mod->len)) {
        vm->flags |= FLAG_FAULT_CODE;
        return VM_FAULT;
    }

    static const void *opcodes[] = {
        [vm_op_noop]   = &&op_noop,

        [vm_op_push]   = &&op_push,
        [vm_op_pushr]  = &&op_pushr,
        [vm_op_pushf]  = &&op_pushf,
        [vm_op_pop]    = &&op_pop,
        [vm_op_popr]   = &&op_popr,
        [vm_op_dupe]   = &&op_dupe,
        [vm_op_swap]   = &&op_swap,
        [vm_op_arg0]   = &&op_arg0,
        [vm_op_arg1]   = &&op_arg1,
        [vm_op_arg2]   = &&op_arg2,
        [vm_op_arg3]   = &&op_arg3,

        [vm_op_not]    = &&op_not,
        [vm_op_and]    = &&op_and,
        [vm_op_or]     = &&op_or,
        [vm_op_xor]    = &&op_xor,
        [vm_op_bnot]   = &&op_bnot,
        [vm_op_band]   = &&op_band,
        [vm_op_bor]    = &&op_bor,
        [vm_op_bxor]   = &&op_bxor,
        [vm_op_bsl]    = &&op_bsl,
        [vm_op_bsr]    = &&op_bsr,

        [vm_op_neg]    = &&op_neg,
        [vm_op_add]    = &&op_add,
        [vm_op_sub]    = &&op_sub,
        [vm_op_mul]    = &&op_mul,
        [vm_op_lmul]   = &&op_lmul,
        [vm_op_div]    = &&op_div,
        [vm_op_rem]    = &&op_rem,

        [vm_op_eq]     = &&op_eq,
        [vm_op_ne]     = &&op_ne,
        [vm_op_gt]     = &&op_gt,
        [vm_op_ge]     = &&op_ge,
        [vm_op_lt]     = &&op_lt,
        [vm_op_le]     = &&op_le,
        [vm_op_cmp]    = &&op_cmp,

        [vm_op_ret]    = &&op_ret,
        [vm_op_call]   = &&op_call,
        [vm_op_load]   = &&op_load,
        [vm_op_jmp]    = &&op_jmp,
        [vm_op_jz]     = &&op_jz,
        [vm_op_jnz]    = &&op_jnz,

        [vm_op_yield]  = &&op_yield,
        [vm_op_reset]  = &&op_reset,
        [vm_op_tsc]    = &&op_tsc,
        [vm_op_fault]  = &&op_fault,

        [vm_op_io]     = &&op_io,
        [vm_op_ios]    = &&op_ios,

        [vm_op_pack]   = &&op_pack,
        [vm_op_unpack] = &&op_unpack,
    };


#define vm_code(arg_type_t)                                             \
    ({                                                                  \
        if (unlikely(vm->ip + sizeof(arg_type_t) > mod->len)) {         \
            vm->flags |= FLAG_FAULT_CODE;                               \
            return VM_FAULT;                                            \
        }                                                               \
        arg_type_t arg = *((const arg_type_t *) (mod->code + vm->ip));  \
        vm->ip += sizeof(arg_type_t);                                   \
        arg;                                                            \
    })

#define vm_stack(i) vm->stack[vm->sp - 1 - (i)]

#define vm_ensure(c)                            \
    do {                                        \
        if (unlikely(vm->sp < c)) {             \
            vm->flags |= FLAG_FAULT_STACK;      \
            return VM_FAULT;                    \
        }                                       \
    } while (false)

#define vm_peek()                               \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_FAULT_STACK;      \
            return VM_FAULT;                    \
        }                                       \
        vm_stack(0);                            \
    })

#define vm_pop()                                \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_FAULT_STACK;      \
            return VM_FAULT;                    \
        }                                       \
        vm->stack[--vm->sp];                    \
    })

#define vm_push(val)                                    \
    ({                                                  \
        if (unlikely(vm->sp >= vm->specs.stack)) {      \
            vm->flags |= FLAG_FAULT_STACK;              \
            return VM_FAULT;                            \
        }                                               \
        vm->stack[vm->sp++] = (val);                    \
    })


    size_t cycles = vm->specs.speed;
    for (size_t i = 0; i < cycles; ++i) {
        vm->tsc++;

        uint8_t opcode = vm_code(uint8_t);
        const void *label = opcodes[opcode];
        if (unlikely(!label)) { vm->flags |= FLAG_FAULT_CODE; return 0; }
        goto *label;

      op_noop: { continue; }

      op_push: { vm_push(vm_code(vm_word)); continue; }
      op_pushr: { vm_push(vm->regs[vm_code(vm_reg)]); continue; }
      op_pushf: { vm_push(vm->flags); continue; }

      op_pop: { vm_pop(); continue; }
      op_popr: { vm->regs[vm_code(vm_reg)] = vm_pop(); continue; }

      op_dupe: { vm_push(vm_peek()); continue; }
      op_swap: {
            vm_ensure(2);
            vm_word tmp = vm_stack(0);
            vm_stack(0) = vm_stack(1);
            vm_stack(1) = tmp;
            continue;
        }

      op_arg0: {
            uint8_t sp = vm_code(vm_reg);
            vm_ensure(sp);
            vm_word tmp = vm_stack(sp);
            vm_stack(sp) = vm->regs[0];
            vm->regs[0] = tmp;
            continue;
        }
      op_arg1: {
            uint8_t sp = vm_code(vm_reg);
            vm_ensure(sp);
            vm_word tmp = vm_stack(sp);
            vm_stack(sp) = vm->regs[1];
            vm->regs[1] = tmp;
            continue;
        }
      op_arg2: {
            uint8_t sp = vm_code(vm_reg);
            vm_ensure(sp);
            vm_word tmp = vm_stack(sp);
            vm_stack(sp) = vm->regs[2];
            vm->regs[2] = tmp;
            continue;
        }
      op_arg3: {
            uint8_t sp = vm_code(vm_reg);
            vm_ensure(sp);
            vm_word tmp = vm_stack(sp);
            vm_stack(sp) = vm->regs[3];
            vm->regs[3] = tmp;
            continue;
        }

      op_not: { vm_ensure(1); vm_stack(0) = !vm_stack(0); continue; }
      op_and: {
            vm_ensure(2);
            vm_stack(1) = vm_stack(1) && vm_stack(0);
            vm_pop();
            continue;
        }
      op_or: {
            vm_ensure(2);
            vm_stack(1) = vm_stack(1) || vm_stack(0);
            vm_pop();
            continue;
        }
      op_xor: {
            vm_ensure(2);
            uint64_t x = vm_stack(0);
            uint64_t y = vm_stack(1);
            vm_stack(1) = (x || y) && !(x && y);
            vm_pop();
            continue;
        }

      op_bnot: { vm_ensure(1); vm_stack(0) = ~vm_stack(0); continue; }
      op_band: { vm_ensure(2); vm_stack(1) &= vm_stack(0); vm_pop(); continue; }
      op_bor: { vm_ensure(2); vm_stack(1) |= vm_stack(0); vm_pop(); continue; }
      op_bxor: { vm_ensure(2); vm_stack(1) ^= vm_stack(0); vm_pop(); continue; }
      op_bsl: {
            vm_ensure(2);
            vm_stack(1) = ((uint64_t)vm_stack(1)) << vm_stack(0);
            vm_pop();
            continue;
        }
      op_bsr: {
            vm_ensure(2);
            vm_stack(1) = ((uint64_t)vm_stack(1)) >> vm_stack(0);
            vm_pop();
            continue;
        }

      op_neg: { vm_ensure(1); vm_stack(0) = -vm_stack(0); continue; }
      op_add: { vm_ensure(2); vm_stack(1) += vm_stack(0); vm_pop(); continue; }
      op_sub: { vm_ensure(2); vm_stack(1) -= vm_stack(0); vm_pop(); continue; }
      op_mul: { vm_ensure(2); vm_stack(1) *= vm_stack(0); vm_pop(); continue; }
      op_lmul: {
            vm_ensure(2);
            int128_t ret = ((int128_t) vm_stack(0)) * ((int128_t) vm_stack(1));
            vm_stack(0) = ret >> 64;
            vm_stack(1) = ret & ((((__int128) 1) << 64) - 1);
            continue;
        }
      op_div: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_FAULT_MATH; return VM_FAULT; }
            vm_stack(1) /= div;
            vm_pop();
            continue;
        }
      op_rem: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_FAULT_MATH; return VM_FAULT; }
            vm_stack(1) %= div;
            vm_pop();
            continue;
        }

      op_eq:  { vm_ensure(2); vm_stack(1) = vm_stack(0) == vm_stack(1); vm_pop(); continue; }
      op_ne:  { vm_ensure(2); vm_stack(1) = vm_stack(0) != vm_stack(1); vm_pop(); continue; }
      op_gt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) >  vm_stack(1); vm_pop(); continue; }
      op_ge:  { vm_ensure(2); vm_stack(1) = vm_stack(0) >= vm_stack(1); vm_pop(); continue; }
      op_lt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) <  vm_stack(1); vm_pop(); continue; }
      op_le:  { vm_ensure(2); vm_stack(1) = vm_stack(0) <= vm_stack(1); vm_pop(); continue; }
      op_cmp: { vm_ensure(2); vm_stack(1) = vm_stack(0) -  vm_stack(1); vm_pop(); continue; }

      op_ret: {
            mod_id mod_id = 0;
            vm_unpack_ret(vm_pop(), &vm->ip, &vm->sbp, &mod_id);
            if (unlikely(mod_id)) return mod_id;
            continue;
        }
      op_call: {
            vm_ip ip = 0; mod_id mod_id = 0;
            vm_unpack(vm_code(vm_word), &mod_id, &ip);
            vm_push(vm_pack_ret(vm->ip, vm->sbp, unlikely(mod_id) ? mod->id : 0));
            vm->ip = ip; vm->sbp = vm->sp;
            if (unlikely(mod_id)) { return mod_id; }
            continue;
        }
      op_load: {
            vm_word mod_id = vm_pop();
            if (unlikely(mod_id > UINT32_MAX)) { vm->flags |= FLAG_FAULT_CODE; return VM_FAULT; }
            vm_reset(vm);
            return mod_id;
        }
      op_jmp: { vm->ip = vm_code(vm_ip); continue; }
      op_jz:  { vm_ip dst = vm_code(vm_ip); if (!vm_pop()) vm->ip = dst; continue; }
      op_jnz: { vm_ip dst = vm_code(vm_ip); if ( vm_pop()) vm->ip = dst; continue; }

      op_reset: { vm_reset(vm); return VM_RESET; }
      op_yield: { return 0; }
      op_tsc: { vm_push(vm->tsc); continue; }
      op_fault: { vm->flags |= FLAG_FAULT_USER; return VM_FAULT; }

      op_io:  {
            vm->io = vm_code(uint8_t);
            vm->flags |= FLAG_IO;
            return 0;
        }
      op_ios: {
            vm->io = vm_pop();
            vm->flags |= FLAG_IO;
            return 0;
        }

      op_pack: {
            vm_ensure(2);
            vm_stack(1) = vm_pack(vm_stack(0), vm_stack(1));
            vm_pop();
            continue;
        }
      op_unpack: {
            uint32_t msb = 0, lsb = 0;
            vm_unpack(vm_pop(), &msb, &lsb);
            vm_push(msb);
            vm_push(lsb);
            continue;
        }
    }

    return 0;

#undef vm_code
#undef vm_stack
#undef vm_ensure
#undef vm_peek
#undef vm_pop
#undef vm_push
}
