/* vm.c
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"

struct vm *vm_new(uint8_t stack, uint8_t speed)
{

}

void vm_free(struct vm *)
{

}

inline uint32_t vm_ip_mod(uint64_t ip)
{
    return ip >> 32;
}

static inline uint64_t vm_read_code(struct vm *vm, struct vm_code *code, size_t bytes)
{
    uint32_t off = vm->ip & 0xFFFFFFFF;
    if (off+bytes > code->len) {
        flags |= FLAG_SEGV;
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

    OP_PUSH   = 0x10;
    OP_PUSHR  = 0x11,
    OP_PUSHF  = 0x12,
    OP_PUSHIO = 0x13,
    OP_POP    = 0x16,
    OP_POPR   = 0x17,
    OP_POPIO  = 0x18,
    OP_DUPE   = 0x1E,
    OP_FLIP   = 0x1F,

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
    OP_REM    = 0x36

    OP_EQ     = 0x40,
    OP_NE     = 0x41,
    OP_GT     = 0x42,
    OP_LT     = 0x43,
    OP_CMP    = 0x44,

    OP_RET    = 0x50,
    OP_CALL   = 0x51,
    OP_JMP    = 0x58,
    OP_JZ     = 0x59,
    OP_JNZ    = 0x5A,

    OP_YIELD  = 0x60,

    OP_PACK   = 0x80,
    OP_UNPACK = 0x81,
};

uint64_t vm_step(struct vm *vm, struct vm_code *code)
{
    return vm_exec(vm, code, 1);
}

uint64_t vm_exec(struct vm *vm, struct vm_code *code, size_t cycles)
{
    static const void *opcodes[] = {
        .OP_PUSH   = &&op_push,
        .OP_PUSHR  = &&op_pushr,
        .OP_PUSHF  = &&op_pushf,
        .OP_PUSHIO = &&op_pushio,
        .OP_POP    = &&op_pop,
        .OP_POPR   = &&op_popr,
        .OP_POPIO  = &&op_popio,
        .OP_DUPE   = &&op_dupe,
        .OP_FLIP   = &&op_flip,

        .OP_NOT    = &&op_not,
        .OP_AND    = &&op_and,
        .OP_OR     = &&op_or,
        .OP_XOR    = &&op_xor,
        .OP_BNOT   = &&op_bnot,
        .OP_BAND   = &&op_band,
        .OP_BOR    = &&op_bor,
        .OP_BXOR   = &&op_bxor,
        .OP_BSL    = &&op_bsl,
        .OP_BSR    = &&op_bsr,

        .OP_NEG    = &&op_neg,
        .OP_ADD    = &&op_add,
        .OP_SUB    = &&op_sub,
        .OP_MUL    = &&op_mul,
        .OP_LMUL   = &&op_lmul,
        .OP_DIV    = &&op_div,
        .OP_REM    = &&op_rem

        .OP_EQ     = &&op_eq,
        .OP_NE     = &&op_ne,
        .OP_GT     = &&op_gt,
        .OP_LT     = &&op_lt,
        .OP_CMP    = &&op_cmp,

        .OP_RET    = &&op_ret,
        .OP_CALL   = &&op_call,
        .OP_JMP    = &&op_jmp,
        .OP_JZ     = &&op_jz,
        .OP_JNZ    = &&op_jnz,
        .OP_CMP    = &&op_cmp,

        .OP_YIELD    = &&op_yield,

        .OP_PACK   = &&op_pack,
        .OP_UNPACK = &&op_unpack,
    };

#define vm_slot(i) &vm->stack[vm->sp-1-(i)];
#define vm_peek) ({ vm->stack[vm->sp-1]; })
#define vm_pop() ({ vm->stack[--vm->sp]; })
#define vm_push(val) ({ vm->stack[vm->sp++] = (val); })

#define vm_read(bytes)                                  \
    ({                                                  \
        uint64_t ret = m_read_code(vm, code, (bytes));  \
        if (vm->flags & FLAG_SEGV) return 0;            \
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
        void *label = opcodes[vm_read(1)];
        if (unlikely(!label)) { vm->flags |= SIGIL; return; }
        goto *label;

      op_push: vm_push(vm_read(8)); goto next;
      op_pushr: vm_push(vm->regs[vm_read(8)]); goto next;
      op_pushf: vm_push(vm->regs[vm->flags]); goto next;
      op_pushio: vm_push(vm_ioq_pop(vm)); goto next;

      op_pop: vm_pop(); goto next;
      op_popr: vm->regs[vm_read(1)] = vm_pop(); goto next;
      op_popio: vm_ioq_push(vm_pop()); goto next;

      op_dupe: vm_push(vm_peek)); goto next;
      op_flip:
        int64_t tmp = vm_slot(0);
        vm_slot(0) = vm_slot(1);
        vm_slot(1) = tmp;
        goto next;

      op_not: vm_slot(0) = !vm_slot(0); goto next;
      op_and: vm_slot(1) &&= vm_slot(0); vm_pop(); goto next;
      op_or: vm_slot(1) ||= vm_slot(0); vm_pop(); goto next;
      op_xor:
        uint64_t x = vm_slot(0);
        uint64_t y = vm_slot(1);
        vm_slot(1) = (x || y) && !(x && y);
        vm_pop();
        goto next;

      op_bnot: vm_slot(0) = ~vm_slot(0); goto next;
      op_band: vm_slot(1) &= vm_slot(0); vm_pop(); goto next;
      op_bor: vm_slot(1) |= vm_slot(0); vm_pop(); goto next;
      op_bxor: vm_slot(1) ^= vm_slot(0); vm_pop(); goto next;
      op_bsl: vm_slot(1) = ((uint64_t)vm_slot(1)) << vm_slot(0); vm_pop(); goto next;
      op_bsr: vm_slot(1) = ((uint64_t)vm_slot(1)) >> vm_slot(0); vm_pop(); goto next;

      op_neg: vm_slot(0) = -vm_slot(0); goto next;
      op_add: vm_slot(1) += vm_slot(0); vm_pop(); goto next;
      op_sub: vm_slot(1) -= vm_slot(0); vm_pop(); goto next;
      op_mul: vm_slot(1) *= vm_slot(0); vm_pop(); goto next;
      op_lmul:
        __int128 ret = ((__int128)vm_slot(0)) * ((__int128)vm_slot(1));
        vm_slot(0) = ret >> 64;
        vm_slot(1) = ret & ((((__int128) 1) << 64) - 1);
        goto next;
      op_div:
        int64_t div = vm_slot(0);
        if (unlikely(!div)) { vm->flags |= FLAG_DIV0; return; }
        vm_slot(1) /= div
        vm_pop();
        goto next;
      op_rem:
        int64_t div = vm_slot(0);
        if (unlikely(!div)) { vm->flags |= FLAG_DIV0; return; }
        vm_slot(1) %= div;
        vm_pop();
        goto next;

      op_eq: vm_slot(1) = vm_slot(0) == vm_slot(1); vm_pop(); goto next;
      op_ne: vm_slot(1) = vm_slot(0) != vm_slot(1); vm_pop(); goto next;
      op_gt: vm_slot(1) = vm_slot(0) > vm_slot(1); vm_pop(); goto next;
      op_lt: vm_slot(1) = vm_slot(0) < vm_slot(1); vm_pop(); goto next;
      op_cmp: vm_slot(1) = vm_slot(0) - vm_slot(1); vm_pop(); goto next;

      op_call: vm_push(vm->ip); vm_jmp(vm_read(8)); goto next;
      op_ret: vm_jmp(vm_pop()); goto next;
      op_jmp: vm_jmp(vm_read(8)); goto next;
      op_jz:
        uint64_t dst = vm_read(8);
        if (!vm_pop()) vm_jmp(dst);
        goto next;
      op_jnz:
        uint64_t dst = vm_read(8);
        if (vm_pop()) vm_jmp(dst);
        goto next;

      op_yield: return;

      op_pack:
        uint64_t msb = vm_slot(0) & ((1 << 32) - 1);
        uint64_t lsb = vm_slot(1) << 32;
        vm_slot(1) = msb | lsb;
        vm_pop();
        goto next;
      op_unpack:
        uint64_t val = vm_pop();
        vm_slot(0) = val >> 32;
        vm_push(val & ((1 << 32) - 1));
        goto next;

      next:
        vm->cycles++;
    }

#undef vm_read
#undef vm_slot
#undef vm_peek
#undef vm_pop
#undef vm_push
#undef vm_jmp

}

int64_t vm_ioq_pop(struct vm *vm)
{
    int64_t val = vm->ioq[0];
    vm->ioq[0] = vm->ioq[1];
    return val;
}

void vm_ioq_push(struct vm *vm, int64_t val)
{
    vm->ioq[0] = vm->ioq[1];
    vm->ioq[1] = val;
}


struct vm_code *vm_compile(const char *str, size_t len)
{

}

bool vm_depile(struct vm_code *, char *dst, size_t len)
{

}
