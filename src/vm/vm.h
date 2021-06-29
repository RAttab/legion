/* vm.h
   Rémi Attab (remi.attab@gmail.com), 16 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

struct mod;
struct text;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef int64_t word_t;
typedef uint8_t reg_t;

typedef uint32_t ip_t;
typedef uint32_t mod_t;
typedef uint16_t mod_id_t;
typedef uint16_t mod_ver_t;


inline mod_t make_mod(mod_id_t id, mod_ver_t ver) { assert(!(id >> 15)); return id << 16 | ver; }
inline mod_id_t mod_id(mod_t mod) { return mod >> 16; }
inline mod_ver_t mod_ver(mod_t mod) { return ((1 << 16) - 1) & mod; }

inline ip_t mod_ip(mod_t mod) { return 1U << 31 | mod; }
inline bool ip_is_mod(ip_t ip) { return ip >> 31; }
inline mod_t ip_mod(ip_t ip) { return ((1U << 31) - 1) & ip;  }


// -----------------------------------------------------------------------------
// vm_atoms
// -----------------------------------------------------------------------------

enum { vm_atom_cap = 16 };
typedef char atom_t[vm_atom_cap];

void vm_atoms_init(void);

word_t vm_atom(const atom_t *);
bool vm_atoms_set(const atom_t *, word_t id);
bool vm_atoms_str(word_t id, atom_t *dst);

inline bool vm_atoms_eq(const atom_t *lhs, const atom_t *rhs)
{
    return !memcmp(lhs, rhs, vm_atom_cap);
}

inline int vm_atoms_cmp(const atom_t *lhs, const atom_t *rhs)
{
    return memcmp(lhs, rhs, vm_atom_cap);
}


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

size_t vm_dbg(struct vm *, char *dst, size_t len);
