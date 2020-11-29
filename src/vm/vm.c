/* vm.c
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"

size_t vm_len(uint8_t stack)
{
    stack = 2 + (8 * stack);
    return sizeof(struct vm) + sizeof(((struct vm *) NULL)->stack[0]) * stack;
}


void vm_init(struct vm *vm, uint8_t stack, uint8_t speed)
{
    vm->specs.stack = 2 + (8 * stack);
    vm->specs.speed = speed;
}

struct vm *vm_new(uint8_t stack, uint8_t speed)
{
    struct vm *vm = alloc_cache(vm_len(stack));
    vm_init(vm, stack, speed);
    return vm;
}

void vm_free(struct vm *vm)
{
    free(vm);
}

inline uint32_t vm_ip_mod(ip_t ip)
{
    return ip >> 32;
}

static inline ip_t vm_read_code(struct vm *vm, struct vm_code *code, size_t bytes)
{
    uint32_t off = vm->ip & 0xFFFFFFFF;
    if (off+bytes > code->len) {
        vm->flags |= FLAG_CODE_FAULT;
        return 0;
    }

    union {
        uint64_t u64;
        uint8_t u8[8];
    } val;

    for (size_t i = 0; i < bytes; ++i) {
        val.u8[7-i] = code->prog[off + i];
    }

    vm->ip += bytes;
    return val.u64;
}

ip_t vm_step(struct vm *vm, struct vm_code *code)
{
    return vm_exec(vm, code, 1);
}

ip_t vm_exec(struct vm *vm, struct vm_code *code)
{
    if (vm->flags & (FLAG_SUSPENDED | FLAG_FAULTS)) return 0;

    static const void *opcodes[] = {
        [OP_PUSH]   = &&op_push,
        [OP_PUSHR]  = &&op_pushr,
        [OP_PUSHF]  = &&op_pushf,
        [OP_POP]    = &&op_pop,
        [OP_POPR]   = &&op_popr,
        [OP_DUPE]   = &&op_dupe,
        [OP_FLIP]   = &&op_flip,

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
        [OP_TSC]    = &&op_tsc,
        [OP_IO]     = &&op_io,
        [OP_IOS]    = &&op_ios,
        [OP_IOR]    = &&op_ior,

        [OP_PACK]   = &&op_pack,
        [OP_UNPACK] = &&op_unpack,
    };

#define vm_stack(i) vm->stack[vm->sp-1-(i)]

#define vm_ensure(c)                            \
    ({                                          \
        if (unlikely(vm->sp < c)) {             \
            vm->flags |= FLAG_STACK_FAULT;      \
            return -1;                          \
        }                                       \
        true;                                   \
    })

#define vm_peek()                               \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_STACK_FAULT;      \
            return -1;                          \
        }                                       \
        vm->stack[vm->sp-1];                    \
    })

#define vm_pop()                                \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_STACK_FAULT;      \
            return -1;                          \
        }                                       \
        vm->stack[--vm->sp];                    \
    })

#define vm_push(val)                                    \
    ({                                                  \
        if (unlikely(vm->sp == vm->specs.stack)) {      \
            vm->flags |= FLAG_STACK_FAULT;              \
            return -1;                                  \
        }                                               \
        vm->stack[vm->sp++] = (val);                    \
    })

#define vm_read(bytes)                                  \
    ({                                                  \
        uint64_t ret = vm_read_code(vm, code, (bytes)); \
        if (vm->flags & FLAG_CODE_FAULT) return -1;     \
        ret;                                            \
    })

#define vm_jmp(_dst_)                                   \
    ({                                                  \
        uint64_t dst = (_dst_);                         \
        vm->ip = dst;                                   \
        if (vm_ip_mod(dst) != code->key) return dst;    \
        true;                                           \
    })

    size_t cycles = 1 << vm->spec.speed;
    for (size_t i = 0; i < cycles; ++i) {
        const void *label = opcodes[vm_read(1)];
        if (unlikely(!label)) { vm->flags |= FLAG_CODE_FAULT; return 0; }
        goto *label;

      op_push: { vm_push(vm_read(8)); goto next; }
      op_pushr: { vm_push(vm->regs[vm_read(1)]); goto next; }
      op_pushf: { vm_push(vm->regs[vm->flags]); goto next; }

      op_pop: { vm_pop(); goto next; }
      op_popr: { vm->regs[vm_read(1)] = vm_pop(); goto next; }

      op_dupe: { vm_push(vm_peek()); goto next; }
      op_flip: {
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
            if (unlikely(!div)) { vm->flags |= FLAG_MATH_FAULT; return -1; }
            vm_stack(1) /= div;
            vm_pop();
            goto next;
        }
      op_rem: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_MATH_FAULT; return -1; }
            vm_stack(1) %= div;
            vm_pop();
            goto next;
        }

      op_eq:  { vm_ensure(2); vm_stack(1) = vm_stack(0) == vm_stack(1); vm_pop(); goto next; }
      op_ne:  { vm_ensure(2); vm_stack(1) = vm_stack(0) != vm_stack(1); vm_pop(); goto next; }
      op_gt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) > vm_stack(1);  vm_pop(); goto next; }
      op_lt:  { vm_ensure(2); vm_stack(1) = vm_stack(0) < vm_stack(1);  vm_pop(); goto next; }
      op_cmp: { vm_ensure(2); vm_stack(1) = vm_stack(0) - vm_stack(1);  vm_pop(); goto next; }

      op_ret: { vm_jmp(vm_pop()); goto next; }
      op_call: { vm_push(vm->ip); vm_jmp(vm_read(8)); goto next; }
      op_load: { ip_t ip = vm_pop(); vm_reset(vm); return ip; }
      op_jmp: { vm_jmp(vm_read(4)); goto next; }
      op_jz: {
            ip_t dest = vm_read(4);
            if (!vm_pop()) vm_jmp(dest);
            goto next;
        }
      op_jnz: {
            ip_t dest = vm_read(4);
            if (vm_pop()) vm_jmp(dest);
            goto next;
        }

      op_yield: { return 0; }
      op_tsc: { vm_push(vm->tsc); }

      op_io:  {
            vm->ior = -1;
            vm->io = vm_read(1);
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
            vm->ior = vm_read(1);
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
            vm_stack(0) lsb;
            vm_push(msb);
            goto next;
        }

      next:
        vm->tsc++;
    }

    return 0;

#undef vm_read
#undef vm_stack
#undef vm_ensure
#undef vm_peek
#undef vm_pop
#undef vm_push
#undef vm_jmp
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
    vm->flags |= FLAG_IO_FAULT;
}

size_t vm_io_read(struct vm *vm, word_t *dst)
{
    if (vm->io > vm_io_cap) { vm_io_fault(vm); return 0; }

    for (size_t i = 0; i < vm->io; ++i)
        dst[i] = vm->stack[--vm->sp];

    return words;
}

void vm_io_write(struct vm *vm, size_t len, const word_t *src)
{
    assert(len <= vm_io_cap);

    // In case of OOM, we want to fill the stack for debugging purposes.
    for (size_t i = 0; i < len; ++i) {
        if (unlikely(vm->sp == vm->specs.stack)) {
            vm->flags |= FLAG_STACK_FAULT;
            return;
        }
        vm->stack[vm->sp++] = src[len-i-1];
    }

    if (vm->ior == 0) vm->stack[vm->sp++] = len;
    else if (vm->ior != -1) vm->reg[vm->ior-1] = len;
}

void vm_reset(struct vm *vm)
{
    memset(vm + sizeof(vm->specs), 0,
            sizeof(*vm) + sizeof(vm->stack[0]) * vm->specs.stack);
}


// -----------------------------------------------------------------------------
// external code
// -----------------------------------------------------------------------------

#include "vm_compiler.c"
