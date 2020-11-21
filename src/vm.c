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
    struct vm *vm = calloc(1, vm_len(stack));
    vm_init(vm, stack, speed);
    return vm;
}

void vm_free(struct vm *vm)
{
    free(vm);
}

inline uint32_t vm_ip_mod(uint64_t ip)
{
    return ip >> 32;
}

static inline uint64_t vm_read_code(struct vm *vm, struct vm_code *code, size_t bytes)
{
    uint32_t off = vm->ip & 0xFFFFFFFF;
    if (off+bytes > code->len) {
        vm->flags |= FLAG_MEMF;
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

enum opcodes
{
    OP_NOOP   = 0x00,

    OP_PUSH   = 0x10,
    OP_PUSHR  = 0x11,
    OP_PUSHF  = 0x12,
    OP_POP    = 0x13,
    OP_POPR   = 0x14,
    OP_DUPE   = 0x18,
    OP_FLIP   = 0x19,

    OP_NOT    = 0x20,
    OP_AND    = 0x21,
    OP_OR     = 0x22,
    OP_XOR    = 0x23,
    OP_BNOT   = 0x28,
    OP_BAND   = 0x29,
    OP_BOR    = 0x2A,
    OP_BXOR   = 0x2B,
    OP_BSL    = 0x2C,
    OP_BSR    = 0x2D,

    OP_NEG    = 0x30,
    OP_ADD    = 0x31,
    OP_SUB    = 0x32,
    OP_MUL    = 0x33,
    OP_LMUL   = 0x34,
    OP_DIV    = 0x35,
    OP_REM    = 0x36,

    OP_EQ     = 0x40,
    OP_NE     = 0x41,
    OP_GT     = 0x42,
    OP_LT     = 0x43,
    OP_CMP    = 0x44,

    OP_RET    = 0x50,
    OP_CALL   = 0x51,
    OP_LOAD   = 0x52,
    OP_JMP    = 0x58,
    OP_JZ     = 0x59,
    OP_JNZ    = 0x5A,

    OP_YIELD  = 0x60,
    OP_READ   = 0x61,
    OP_WRITE  = 0x62,

    OP_PACK   = 0x80,
    OP_UNPACK = 0x81,
};

uint64_t vm_step(struct vm *vm, struct vm_code *code)
{
    return vm_exec(vm, code, 1);
}

uint64_t vm_exec(struct vm *vm, struct vm_code *code, size_t cycles)
{
    if (vm->flags & FLAG_SUSPENDED) return 0;

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
        [OP_READ]   = &&op_read,
        [OP_WRITE]  = &&op_write,

        [OP_PACK]   = &&op_pack,
        [OP_UNPACK] = &&op_unpack,
    };

#define vm_stack(i) vm->stack[vm->sp-1-(i)]

#define vm_ensure(c)                            \
    ({                                          \
        if (unlikely(vm->sp < c)) {             \
            vm->flags |= FLAG_OOM;              \
            return -1;                          \
        }                                       \
        true;                                   \
    })

#define vm_peek()                               \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_OOM;              \
            return -1;                          \
        }                                       \
        vm->stack[vm->sp-1];                    \
    })

#define vm_pop()                                \
    ({                                          \
        if (unlikely(!vm->sp)) {                \
            vm->flags |= FLAG_OOM;              \
            return -1;                          \
        }                                       \
        vm->stack[--vm->sp];                    \
    })

#define vm_push(val)                                    \
    ({                                                  \
        if (unlikely(vm->sp == vm->specs.stack)) {      \
            vm->flags |= FLAG_OOM;                      \
            return -1;                                  \
        }                                               \
        vm->stack[vm->sp++] = (val);                    \
    })

#define vm_read(bytes)                                  \
    ({                                                  \
        uint64_t ret = vm_read_code(vm, code, (bytes)); \
        if (vm->flags & FLAG_MEMF) return -1;           \
        ret;                                            \
    })

#define vm_jmp(_dst_)                                   \
    ({                                                  \
        uint64_t dst = (_dst_);                         \
        vm->ip = dst;                                   \
        if (vm_ip_mod(dst) != code->key) return dst;    \
        true;                                           \
    })

    for (size_t i = 0; i < cycles; ++i) {
        const void *label = opcodes[vm_read(1)];
        if (unlikely(!label)) { vm->flags |= FLAG_OPF; return 0; }
        goto *label;

      op_push: { vm_push(vm_read(8)); goto next; }
      op_pushr: { vm_push(vm->regs[vm_read(8)]); goto next; }
      op_pushf: { vm_push(vm->regs[vm->flags]); goto next; }

      op_pop: { vm_pop(); goto next; }
      op_popr: { vm->regs[vm_read(1)] = vm_pop(); goto next; }

      op_dupe: { vm_push(vm_peek()); goto next; }
      op_flip: {
            vm_ensure(2);
            int64_t tmp = vm_stack(0);
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
            if (unlikely(!div)) { vm->flags |= FLAG_DIV0; return -1; }
            vm_stack(1) /= div;
            vm_pop();
            goto next;
        }
      op_rem: {
            vm_ensure(2);
            int64_t div = vm_stack(0);
            if (unlikely(!div)) { vm->flags |= FLAG_DIV0; return -1; }
            vm_stack(1) %= div;
            vm_pop();
            goto next;
        }

      op_eq: { vm_ensure(2); vm_stack(1) = vm_stack(0) == vm_stack(1); vm_pop(); goto next; }
      op_ne: { vm_ensure(2); vm_stack(1) = vm_stack(0) != vm_stack(1); vm_pop(); goto next; }
      op_gt: { vm_ensure(2); vm_stack(1) = vm_stack(0) > vm_stack(1); vm_pop(); goto next; }
      op_lt: { vm_ensure(2); vm_stack(1) = vm_stack(0) < vm_stack(1); vm_pop(); goto next; }
      op_cmp: { vm_ensure(2); vm_stack(1) = vm_stack(0) - vm_stack(1); vm_pop(); goto next; }

      op_ret: { vm_jmp(vm_pop()); goto next; }
      op_call: { vm_push(vm->ip); vm_jmp(vm_read(8)); goto next; }
      op_load: { uint64_t ip = vm_pop(); vm_reset(vm); return ip; }
      op_jmp: { vm_jmp(vm_read(8)); goto next; }
      op_jz: {
            uint64_t dest = vm_read(8);
            if (!vm_pop()) vm_jmp(dest);
            goto next;
        }
      op_jnz: {
            uint64_t dest = vm_read(8);
            if (vm_pop()) vm_jmp(dest);
            goto next;
        }

      op_yield: { return 0; }
      op_read: {
            vm_push(vm_read(1));
            vm->flags |= FLAG_READING;
            return 0;
        }
      op_write: {
            vm_push(vm_read(1));
            vm->flags |= FLAG_WRITING;
            return 0;
        }

      op_pack: {
            vm_ensure(2);
            uint64_t msb = vm_stack(0) & -1U;
            uint64_t lsb = vm_stack(1) << 32;
            vm_stack(1) = msb | lsb;
            vm_pop();
            goto next;
        }
      op_unpack: {
            uint64_t val = vm_pop();
            vm_stack(0) = val >> 32;
            vm_push(val & -1U);
            goto next;
        }

      next:
        vm->cycles++;
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
    vm->flags |= FLAG_SUSPENDED | FLAG_IOF;
}

size_t vm_io_read(struct vm *vm, int64_t *dst)
{
    size_t words = vm->stack[--vm->sp];
    if (words > vm_io_cap) { vm_io_fault(vm); return 0; }

    for (size_t i = 0; i < words; ++i)
        dst[i] = vm->stack[--vm->sp];

    return words;
}

void vm_io_write(struct vm *vm, size_t len, const int64_t *src)
{
    assert(len <= vm_io_cap);

    // In case of OOM, we want to fill the stack for debugging purposes.
    for (size_t i = 0; i < len; ++i) {
        if (unlikely(vm->sp == vm->specs.stack)) {
            vm->flags |= FLAG_OOM;
            return;
        }
        vm->stack[vm->sp++] = src[i];
    }
}

void vm_reset(struct vm *vm)
{
    memset(vm + sizeof(vm->specs), 0,
            sizeof(*vm) + sizeof(vm->stack[0]) * vm->specs.stack);
}

struct vm_code *vm_compile(const char *str, size_t len)
{
    (void) str;
    (void) len;
    return NULL;
}
