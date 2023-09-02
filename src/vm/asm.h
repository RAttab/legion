/* asm.h
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "vm/op.h"
#include "utils/symbol.h"

struct mod;


// -----------------------------------------------------------------------------
// assembly
// -----------------------------------------------------------------------------

struct assembly;

struct assembly *asm_alloc(void);
void asm_free(struct assembly *);
void asm_reset(struct assembly *);

bool asm_empty(const struct assembly *);
uint32_t asm_rows(const struct assembly *);
vm_ip asm_ip(const struct assembly *, uint32_t row);
uint32_t asm_row(const struct assembly *, vm_ip);


struct asm_line
{
    uint32_t row;

    vm_ip ip;
    enum vm_op op;
    enum vm_op_arg arg;

    vm_word value;
    struct symbol symbol;
};
typedef const struct asm_line *asm_it;
asm_it asm_at(const struct assembly *, uint32_t row);


struct asm_jmp_it
{
    uint8_t layer;
    uint32_t src, dst;
    uint32_t it, min, max;
};
struct asm_jmp_it asm_jmp_begin(
        const struct assembly *, uint32_t min, uint32_t max);
bool asm_jmp_step(const struct assembly *, struct asm_jmp_it *);


struct asm_jmp
{
    uint8_t layer;
    uint32_t src, dst; // line index
};
typedef const struct asm_jmp *asm_jmp_it;
asm_jmp_it asm_jmp_from(const struct assembly *, uint32_t row);
asm_jmp_it asm_jmp_to(const struct assembly *, uint32_t row);


void asm_parse(struct assembly *, const struct mod *);
void asm_dump(struct assembly *);
