/* vm.h
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

// -----------------------------------------------------------------------------
// code
// -----------------------------------------------------------------------------

struct vm_code
{
    uin32_t key;

    uint8_t len;
    uint8_t prog[];
};

struct legion_packed vm
{
    uint8_t stack;
    uint8_t speed;
    uint32_t cycles;
    uint8_t flags;
    uint8_t sp;
    uint64_t ip;
    uint64_t regs[4];
    uint64_t stack[];
};

static_assert(sizeof(vm) % 8 == 0);

struct vm *vm_new(uint8_t stack, uint8_t speed);
void vm_free(struct vm *);

uint64_t vm_exec(struct vm *, struct vm_code *, size_t cycles);

struct vm_code *vm_compile(const char *str, size_t len);
bool vm_depile(struct vm_code *, char *dst, size_t len);

inline uint32_t vm_ip_mod(uint64_t ip);
