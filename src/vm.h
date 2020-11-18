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

    FLAG_READING = 1 << 6,
    FLAG_WRITING = 1 << 7,
};

struct vm_code
{
    uint32_t key;

    uint8_t len;
    uint8_t prog[];
};

struct legion_packed vm
{
    struct legion_packed {
        uint8_t stack;
        uint8_t speed;
    } specs;

    uint32_t cycles;
    uint8_t flags;
    uint8_t sp;
    uint64_t ip;
    int64_t regs[4];
    int64_t stack[];
};

static_assert(sizeof(struct vm) % 8 == 0);

struct vm *vm_new(uint8_t stack, uint8_t speed);
void vm_free(struct vm *);

inline uint32_t vm_ip_mod(uint64_t ip);

uint64_t vm_exec(struct vm *, struct vm_code *, size_t cycles);
uint64_t vm_step(struct vm *, struct vm_code *);

size_t vm_io_read(struct vm *, size_t len, int64_t *dst);
void vm_io_push(struct vm *, size_t len, const int64_t *src);
void vm_reset(struct vm *);

struct vm_code *vm_compile(const char *str, size_t len);

