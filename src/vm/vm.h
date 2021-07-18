/* vm.h
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/types.h"


// -----------------------------------------------------------------------------
// vm
// -----------------------------------------------------------------------------

enum flags
{
    FLAG_IO          = 1 << 0,
    FLAG_SUSPENDED   = 1 << 1,
    FLAG_FAULT_USER  = 1 << 2,
    FLAG_FAULT_REG   = 1 << 3,
    FLAG_FAULT_STACK = 1 << 4,
    FLAG_FAULT_CODE  = 1 << 5,
    FLAG_FAULT_MATH  = 1 << 6,
    FLAG_FAULT_IO    = 1 << 7,
};

struct legion_packed vm
{
    struct legion_packed {
        uint8_t stack; // = 2 + type * 8
        uint8_t speed;
    } specs;
    uint8_t flags, sp; // u16 - u32
    uint32_t tsc; // -> u64

    uint8_t io, ior;
    legion_pad(2);

    ip_t ip;
    word_t regs[4]; // half of the cacheline
    word_t stack[]; // 2 u64 left in cacheline
};

static_assert(sizeof(struct vm) == 6*8);

// These macros are needed in enum computation so can't be functions
#define vm_stack_len(stack) (2 + (8 * (stack)))
#define vm_len(stack) (sizeof(struct vm) + vm_stack_len(stack) * sizeof(word_t))

struct vm *vm_alloc(uint8_t stack, uint8_t speed);
void vm_free(struct vm *);

void vm_init(struct vm *, uint8_t stack, uint8_t speed);

static const ip_t VM_FAULT = -1;
ip_t vm_exec(struct vm *, const struct mod *);

void vm_reset(struct vm *);
void vm_suspend(struct vm *);
void vm_resume(struct vm *);
void vm_io_fault(struct vm *);
inline bool vm_io(struct vm *vm) { return vm->flags & FLAG_IO; }

enum { vm_io_cap = 8 };
typedef word_t vm_io_buf_t[vm_io_cap];

void vm_push(struct vm *, word_t);

size_t vm_io_read(struct vm *, word_t *dst);
inline bool vm_io_check(struct vm *vm, size_t len, size_t exp)
{
    if (unlikely(len < exp)) { vm_io_fault(vm); return false; }
    return true;
}

inline word_t vm_pack(uint32_t msb, uint32_t lsb)
{
    return (((uint64_t) msb) << 32) | lsb;
}
inline void vm_unpack(word_t in, uint32_t *msb, uint32_t *lsb)
{
    *msb = ((uint64_t) in) >> 32;
    *lsb = (uint32_t) in;
}

size_t vm_dbg(struct vm *, char *dst, size_t len);
