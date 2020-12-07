/* vm.c
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"
#include "vm/op.h"
#include "vm/mod.h"

#ifdef VM_DEBUG
# define vm_assert(p) assert(p)
#else
# define vm_assert(p)
#endif

// -----------------------------------------------------------------------------
// flags
// -----------------------------------------------------------------------------

static uint8_t flag_faults =
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

size_t vm_len(uint8_t stack)
{
    stack = 2 + (8 * stack);
    return sizeof(struct vm) + sizeof(((struct vm *) NULL)->stack[0]) * stack;
}


void vm_init(struct vm *vm, uint8_t stack, uint8_t speed)
{
    vm->specs.stack = 2 + (8 * stack);
    vm->specs.speed = 1 << speed;
}

void vm_suspend(struct vm *vm)
{
    vm->flags |= FLAG_SUSPENDED;
}

void vm_resume(struct vm *vm)
{
    vm->flags &= ~FLAG_SUSPENDED;
}

void vm_io_fault(struct vm *vm)
{
    vm->flags |= FLAG_FAULT_IO;
}

size_t vm_io_read(struct vm *vm, word_t *dst)
{
    if (vm->io > vm_io_cap) { vm_io_fault(vm); return 0; }

    for (size_t i = 0; i < vm->io; ++i)
        dst[i] = vm->stack[--vm->sp];

    return vm->io;
}

void vm_io_write(struct vm *vm, size_t len, const word_t *src)
{
    assert(len <= vm_io_cap);

    // In case of OOM, we want to fill the stack for debugging purposes.
    for (size_t i = 0; i < len; ++i) {
        if (unlikely(vm->sp == vm->specs.stack)) {
            vm->flags |= FLAG_FAULT_STACK;
            return;
        }
        vm->stack[vm->sp++] = src[len-i-1];
    }

    if (vm->ior == 0) vm->stack[vm->sp++] = len;
    else if (vm->ior != 0xFF) vm->regs[vm->ior-1] = len;
}

void vm_reset(struct vm *vm)
{
    mod_t mod = ip_mod(vm->ip);
    uint8_t stack = vm->specs.stack;
    uint8_t speed = vm->specs.speed;

    memset(vm, 0, sizeof(*vm) + sizeof(vm->stack[0]) * vm->specs.stack);

    vm->specs.stack = stack;
    vm->specs.speed = speed;
    vm->ip = make_ip(mod, 0);
}

size_t vm_dbg(struct vm *vm, char *dst, size_t len)
{
    size_t orig = len;

    size_t n = 0;
    n = snprintf(dst, len, "spec:  { stack:%u, speed:%u }\n",
            (unsigned) vm->specs.stack, (unsigned) vm->specs.speed);
    dst += n; len -= n;

    n = snprintf(dst, len, "gen:   { ip=%08x, sp=%02x, flags:%02x, io:{n:%02x, r:%02x}, tsc:%08x }\n",
            vm->ip, (unsigned) vm->sp, (unsigned) vm->flags,
            (unsigned) vm->io, (unsigned) vm->ior, vm->tsc);
    dst += n; len -= n;

    n = snprintf(dst, len, "reg:   [ 1:%016lx 2:%016lx 3:%016lx 4:%016lx ]\n",
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


ip_t vm_exec(struct vm *vm, const struct mod *mod)
{
    if (vm->flags & (FLAG_SUSPENDED | flag_faults)) return 0;
    vm_assert(ip_addr(vm->ip) < mod->len);
    assert(ip_mod(vm->ip) == mod->id);

    static const void *opcodes[] = {
        [OP_PUSH]   = &&op_push,
        [OP_PUSHR]  = &&op_pushr,
        [OP_PUSHF]  = &&op_pushf,
        [OP_POP]    = &&op_pop,
        [OP_POPR]   = &&op_popr,
        [OP_DUPE]   = &&op_dupe,
        [OP_SWAP]   = &&op_swap,

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
        [OP_LT]     = &&op_lt,
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
        [OP_IO]     = &&op_io,
        [OP_IOS]    = &&op_ios,
        [OP_IOR]    = &&op_ior,

        [OP_PACK]   = &&op_pack,
        [OP_UNPACK] = &&op_unpack,
    };


#define vm_code(arg_type_t)                                             \
    ({                                                                  \
        addr_t off = ip_addr(vm->ip);                                   \
        vm_assert(off + sizeof(arg_type_t) <= mod->len);                \
        arg_type_t arg = *((const arg_type_t *) (mod->code + off));     \
        vm->ip += sizeof(arg_type_t);                                   \
        arg;                                                            \
    })

#define vm_stack(i) vm->stack[vm->sp - 1 - (i)]

#define vm_ensure(c)                            \
    do {                                        \
        if (unlikely(vm->sp < c)) {             \
            vm->flags |= FLAG_FAULT_STACK;      \
            return -1;                          \
        }                                       \
    } while (false)

#define vm_peek()                               \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_FAULT_STACK;      \
            return -1;                          \
        }                                       \
        vm_stack(0);                            \
    })

#define vm_pop()                                \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_FAULT_STACK;      \
            return -1;                          \
        }                                       \
        vm->stack[--vm->sp];                    \
    })

#define vm_push(val)                                    \
    ({                                                  \
        if (unlikely(vm->sp >= vm->specs.stack)) {      \
            vm->flags |= FLAG_FAULT_STACK;              \
            return -1;                                  \
        }                                               \
        vm->stack[vm->sp++] = (val);                    \
    })

#define vm_jmp(_target_)                        \
    do {                                        \
        addr_t target = (_target_);             \
        vm->ip = make_ip(mod->id, target);      \
    } while (false)


    size_t cycles = vm->specs.speed;
    for (size_t i = 0; i < cycles; ++i) {

        uint8_t opcode = vm_code(uint8_t);
        const void *label = opcodes[opcode];
        vm_assert(label);
        goto *label;

      op_push: { vm_push(vm_code(word_t)); goto next; }
      op_pushr: { vm_push(vm->regs[vm_code(reg_t)]); goto next; }
      op_pushf: { vm_push(vm->flags); goto next; }

      op_pop: { vm_pop(); goto next; }
      op_popr: { vm->regs[vm_code(reg_t)] = vm_pop(); goto next; }

      op_dupe: { vm_push(vm_peek()); goto next; }
      op_swap: {
            vm_ensure(2);
            word_t tmp = vm_stack(0);
            vm_stack(0) = vm_stack(1);
            vm_stack(1) = tmp;
            goto next;
        }

      op_not: { vm_ensure(1); vm_stack(0) = !vm_stack(0); goto next; }
      op_and: {
            vm_ensure(2);
            vm_stack(1) = vm_stack(1) && vm_stack(0);
            vm_pop();
            goto next;
        }
      op_or: {
            vm_ensure(2);
            vm_stack(1) = vm_stack(1) || vm_stack(0);
            vm_pop();
            goto next;
        }
      op_xor: {
            vm_ensure(2);
            uint64_t x = vm_stack(0);
            uint64_t y = vm_stack(1);
            vm_stack(1) = (x || y) && !(x && y);
            vm_pop();
            goto next;
        }

      op_bnot: { vm_ensure(1); vm_stack(0) = ~vm_stack(0); goto next; }
      op_band: { vm_ensure(2); vm_stack(1) &= vm_stack(0); vm_pop(); goto next; }
      op_bor: { vm_ensure(2); vm_stack(1) |= vm_stack(0); vm_pop(); goto next; }
      op_bxor: { vm_ensure(2); vm_stack(1) ^= vm_stack(0); vm_pop(); goto next; }
      op_bsl: {
            vm_ensure(2);
            vm_stack(1) = ((uint64_t)vm_stack(1)) << vm_stack(0);
            vm_pop();
            goto next;
        }
      op_bsr: {
            vm_ensure(2);
            vm_stack(1) = ((uint64_t)vm_stack(1)) >> vm_stack(0);
            vm_pop();
            goto next;
        }

      op_neg: { vm_ensure(1); vm_stack(0) = -vm_stack(0); goto next; }
      op_add: { vm_ensure(2); vm_stack(1) += vm_stack(0); vm_pop(); goto next; }
      op_sub: { vm_ensure(2); vm_stack(1) -= vm_stack(0); vm_pop(); goto next; }
      op_mul: { vm_ensure(2); vm_stack(1) *= vm_stack(0); vm_pop(); goto next; }
      op_lmul: {
            vm_ensure(2);
            __int128 ret = ((__int128)vm_stack(0)) * ((__int128)vm_stack(1));
            vm_stack(0) = ret >> 64;
            vm_stack(1) = ret & ((((__int128) 1) << 64) - 1);
            goto next;
        }
      op_div: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_FAULT_MATH; return -1; }
            vm_stack(1) /= div;
            vm_pop();
            goto next;
        }
      op_rem: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_FAULT_MATH; return -1; }
            vm_stack(1) %= div;
            vm_pop();
            goto next;
        }

      op_eq:  { vm_ensure(2); vm_stack(1) = vm_stack(0) == vm_stack(1); vm_pop(); goto next; }
      op_ne:  { vm_ensure(2); vm_stack(1) = vm_stack(0) != vm_stack(1); vm_pop(); goto next; }
      op_gt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) > vm_stack(1);  vm_pop(); goto next; }
      op_lt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) < vm_stack(1);  vm_pop(); goto next; }
      op_cmp: { vm_ensure(2); vm_stack(1) = vm_stack(0) - vm_stack(1);  vm_pop(); goto next; }

      op_ret: {
            word_t raw = vm_pop();
            if (raw > ((ip_t)-1)) { vm->flags |= FLAG_FAULT_CODE; return -1; }
            ip_t ip = raw;
            if (ip_mod(ip) == mod->id) { vm_jmp(ip_addr(ip)); goto next; }
            return ip;
        }
      op_call: {
            ip_t ip = vm_code(ip_t);
            vm_push(vm->ip);

            if (!ip_mod(ip)) { vm_jmp(ip_addr(ip)); goto next; }
            vm_assert(!ip_addr(ip));
            return ip;
        }
      op_load: { ip_t ip = vm_pop(); vm_reset(vm); return ip; }
      op_jmp: { vm_jmp(vm_code(ip_t)); goto next; }
      op_jz:  { ip_t dst = vm_code(ip_t); if (!vm_pop()) vm_jmp(dst); goto next; }
      op_jnz: { ip_t dst = vm_code(ip_t); if ( vm_pop()) vm_jmp(dst); goto next; }

      op_reset: { vm_reset(vm); return 0; }
      op_yield: { return 0; }
      op_tsc: { vm_push(vm->tsc); }

      op_io:  {
            vm->ior = 0xFF;
            vm->io = vm_code(uint8_t);
            vm->flags |= FLAG_IO;
            return 0;
        }
      op_ios: {
            vm->ior = 0;
            vm->io = vm_pop();
            vm->flags |= FLAG_IO;
            return 0;
        }
      op_ior: {
            vm->ior = vm_code(reg_t) + 1;
            vm->io = vm->regs[vm->ior - 1];
            vm->flags |= FLAG_IO;
            return 0;
        }

      op_pack: {
            vm_ensure(2);
            vm_stack(1) = vm_pack(vm_stack(0), vm_stack(1));
            vm_pop();
            goto next;
        }
      op_unpack: {
            uint32_t msb = 0, lsb = 0;
            vm_unpack(vm_pop(), &msb, &lsb);
            vm_stack(0) = lsb;
            vm_push(msb);
            goto next;
        }

      next:
        vm->tsc++;
    }

    return 0;

#undef vm_code
#undef vm_stack
#undef vm_ensure
#undef vm_peek
#undef vm_pop
#undef vm_push
#undef vm_jmp
}
