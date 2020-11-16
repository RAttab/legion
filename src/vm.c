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

enum opcodes
{
    OP_PUSH = 0;
    OP_PUSHR = 1,

    OP_POP = 2,
    OP_POPR = 3,
};

enum flags
{
    FLAG_SEGV = 1 << 0,
    FLAG_DIV0 = 1 << 1,
};

inline uint32_t vm_ip_mod(uint64_t ip)
{
    return ip >> 32;
}

static inline uint64_t vm_read(struct vm *vm, struct vm_code *code, size_t bytes)
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

uint64_t vm_exec(struct vm *vm, struct vm_code *code, size_t cycles)
{
    static const void *opcodes[] = {
        .OP_PUSH  = &&op_push,
        .OP_PUSHR = &&op_pushr,
        .OP_POP   = &&op_pop,
        .OP_POPR  = &&op_popr,
    };

#define inc_ip(bytes)                                   \
    ({                                                  \
        uint64_t ret = vm_read(vm, code, (bytes));      \
        if (vm->flags & FLAG_SEGV) return 0;            \
        ret;                                            \
    })
    

    for (size_t i = 0; i < cycles; ++i) {
        goto *opcodes[inc_ip(1)];

      op_push:
        uint64_t literal = inc_ip(8);
        vm->stack[vm->sp++] = literal;
        goto done;

      op_pushr:
        uint8_t reg = inc_ip(8);
        vm->stack[vm->sp++] = vm->regs[reg];
        goto done;

      op_pop:
        vm->sp--;
        goto done;
        
      op_popr:
        uint8_t reg = inc_ip(vm, code, 1);
        vm->stack[vm->sp++] = vm->regs[reg];
        goto done;

      done:
        vm->cycles++;
    }
}


struct vm_code *vm_compile(const char *str, size_t len)
{

}

bool vm_depile(struct vm_code *, char *dst, size_t len)
{

}
