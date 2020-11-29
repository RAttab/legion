/* vm.h
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct text;

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef int64_t word_t;

typedef uint32_t ip_t;
typedef uint16_t mod_t;
typedef uint16_t off_t;

inline ip_t make_ip(mod_t mod, off_t off) { return (((uint32_t) mod) << 16) | off; }
inline mod_t ip_mod(ip_t ip) { return ip >> 16; }
inline ip_t ip_off(ip_t ip) { return (uint16_t) ip; }


// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

enum { vm_errors_cap = 32 };
struct vm_errors
{
    size_t line, col;
    char err[vm_errors_cap];
}

struct vm_code
{
    uint32_t mod;

    char *str;
    size_t str_len;

    struct vm_errors *errs;
    size_t errs_len;

    uint8_t len;
    uint8_t prog[];
};

void vm_compile_init(void);
struct vm_code *vm_compile(const char *name, struct text *source);


// -----------------------------------------------------------------------------
// vm
// -----------------------------------------------------------------------------

enum flags
{
    FLAG_IO = 1 << 0,
    FLAG_SUSPENDED  = 1 << 1,

    FLAG_FAULT_REG = 1 << 3,
    FLAG_FAULT_STACK = 1 << 4,
    FLAG_FAULT_CODE = 1 << 5,
    FLAG_FAULT_MATH = 1 << 6,
    FLAG_FAULT_IO  = 1 << 7,

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
    uint8_t __pad__[2];

    ip_t ip;
    word_t regs[4]; // half of the cacheline
    word_t stack[]; // 2 u64 left in cacheline
};

static_assert(sizeof(struct vm) == 6*8);

struct vm *vm_alloc(uint8_t stack, uint8_t speed);
void vm_free(struct vm *);

void vm_init(struct vm *, uint8_t stack, uint8_t speed);
size_t vm_len(uint8_t stack);

inline uint32_t vm_ip_mod(uint64_t ip);

ip_t vm_exec(struct vm *, struct vm_code *);
ip_t vm_step(struct vm *, struct vm_code *);

void vm_suspend(struct vm *);
void vm_resume(struct vm *);
void vm_io_fault(struct vm *);

inline bool vm_io(struct vm *vm) { return vm->flags & FLAG_IO; }

enum { vm_io_cap = 8 };
typedef word_t vm_io_buf_t[vm_io_cap];

size_t vm_io_read(struct vm *, word_t *dst);
void vm_io_write(struct vm *, size_t len, const word_t *src);

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

void vm_reset(struct vm *);

