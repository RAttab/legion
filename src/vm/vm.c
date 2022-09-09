/* vm.c
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"
#include "vm/op.h"
#include "vm/mod.h"

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
    struct vm *vm = alloc_cache(vm_len(stack));
    vm_init(vm, stack, speed);
    return vm;
}

void vm_free(struct vm *vm)
{
    free(vm);
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

    n = snprintf(dst, len, "gen:   { ip=%08x, sp=%02x, flags:%02x, io:%02x }, tsc:%08x }\n",
            vm->ip, (unsigned) vm->sp, (unsigned) vm->flags, (unsigned) vm->io, vm->tsc);
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
        [OP_NOOP]   = &&op_noop,

        [OP_PUSH]   = &&op_push,
        [OP_PUSHR]  = &&op_pushr,
        [OP_PUSHF]  = &&op_pushf,
        [OP_POP]    = &&op_pop,
        [OP_POPR]   = &&op_popr,
        [OP_DUPE]   = &&op_dupe,
        [OP_SWAP]   = &&op_swap,
        [OP_ARG0]   = &&op_arg0,
        [OP_ARG1]   = &&op_arg1,
        [OP_ARG2]   = &&op_arg2,
        [OP_ARG3]   = &&op_arg3,

        [OP_NOT]    = &&op_not,
        [OP_AND]    = &&op_and,
        [OP_OR]     = &&op_or,
        [OP_XOR]    = &&op_xor,
        [OP_BNOT]   = &&op_bnot,
        [OP_BAND]   = &&op_band,
        [OP_BOR]    = &&op_bor,
        [OP_BXOR]   = &&op_bxor,
        [OP_BSL]    = &&op_bsl,
        [OP_BSR]    = &&op_bsr,

        [OP_NEG]    = &&op_neg,
        [OP_ADD]    = &&op_add,
        [OP_SUB]    = &&op_sub,
        [OP_MUL]    = &&op_mul,
        [OP_LMUL]   = &&op_lmul,
        [OP_DIV]    = &&op_div,
        [OP_REM]    = &&op_rem,

        [OP_EQ]     = &&op_eq,
        [OP_NE]     = &&op_ne,
        [OP_GT]     = &&op_gt,
        [OP_GE]     = &&op_ge,
        [OP_LT]     = &&op_lt,
        [OP_LE]     = &&op_le,
        [OP_CMP]    = &&op_cmp,

        [OP_RET]    = &&op_ret,
        [OP_CALL]   = &&op_call,
        [OP_LOAD]   = &&op_load,
        [OP_JMP]    = &&op_jmp,
        [OP_JZ]     = &&op_jz,
        [OP_JNZ]    = &&op_jnz,

        [OP_YIELD]  = &&op_yield,
        [OP_RESET]  = &&op_reset,
        [OP_TSC]    = &&op_tsc,
        [OP_FAULT]  = &&op_fault,

        [OP_IO]     = &&op_io,
        [OP_IOS]    = &&op_ios,

        [OP_PACK]   = &&op_pack,
        [OP_UNPACK] = &&op_unpack,
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
            mod_id mod_id = 0; vm_ip ip = 0;
            vm_unpack(vm_pop(), &mod_id, &ip);
            vm->ip = ip;
            if (unlikely(mod_id)) return mod_id;
            continue;
        }
      op_call: {
            mod_id mod_id = 0; vm_ip ip = 0;
            vm_unpack(vm_code(vm_word), &mod_id, &ip);
            vm_push(vm_pack(unlikely(mod_id) ? mod->id : 0, vm->ip));
            vm->ip = ip;
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
