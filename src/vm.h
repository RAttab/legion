/* vm.h
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

enum flags
{
    FLAG_DIV0 = 1 << 0,
    FLAG_OPF  = 1 << 1,
    FLAG_MEMF = 1 << 2,
    FLAG_OOM  = 1 << 3,
    FLAG_IOF  = 1 << 4,

    FLAG_SUSPENDED  = 1 << 5,
    FLAG_READING = 1 << 6,
    FLAG_WRITING = 1 << 7,
};

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

struct legion_packed vm
{
    struct legion_packed {
        uint8_t stack; // = 2 + type * 8
        uint8_t speed;
    } specs;
    uint8_t flags;
    uint8_t sp;
    uint32_t cycles;
    uint64_t ip;     // ^- sum(*) = 2 u64
    int64_t regs[4]; // half of the cacheline
    int64_t stack[]; // 2 u64 left in cacheline
};

static_assert(sizeof(struct vm) % 8 == 0);

struct vm *vm_new(uint8_t stack, uint8_t speed);
void vm_init(struct vm *, uint8_t stack, uint8_t speed);
void vm_free(struct vm *);
size_t vm_len(uint8_t stack);

inline uint32_t vm_ip_mod(uint64_t ip);

uint64_t vm_exec(struct vm *, struct vm_code *, size_t cycles);
uint64_t vm_step(struct vm *, struct vm_code *);

void vm_suspend(struct vm *);
void vm_resume(struct vm *);
void vm_io_fault(struct vm *);

inline bool vm_reading(struct vm *vm) { return vm->flags & FLAG_READING; }
inline bool vm_writing(struct vm *vm) { return vm->flags & FLAG_WRITING; }

enum { vm_io_cap = 8 };
typedef int64_t vm_io_buf_t[vm_io_cap];

size_t vm_io_read(struct vm *, int64_t *dst);
void vm_io_write(struct vm *, size_t len, const int64_t *src);

inline bool vm_io_check(struct vm *vm, size_t len, size_t exp)
{
    if (unlikely(len < exp)) { vm_io_fault(vm); return false; }
    return true;
}

void vm_reset(struct vm *);

void vm_compile_init();
struct vm_code *vm_compile(const char *str, size_t len);
