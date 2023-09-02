/* opx.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply

   XMacro header:

   vm_op_fn(op, str, arg) ->
   vm_op_fn(vm_op_ ## op, #str, vm_op_arg_ ## nil | lit | reg | off | mod)
*/

#ifndef vm_op_fn
#  error "vm_op_fn must be declared when including op_xmacro.h"
#endif

vm_op_fn(vm_op_noop, NOOP, nil)

vm_op_fn(vm_op_push, PUSH, lit)
vm_op_fn(vm_op_pushr, PUSHR, reg)
vm_op_fn(vm_op_pushf, PUSHF, nil)
vm_op_fn(vm_op_pop, POP, nil)
vm_op_fn(vm_op_popr, POPR, reg)
vm_op_fn(vm_op_dupe, DUPE, nil)
vm_op_fn(vm_op_swap, SWAP, nil)
vm_op_fn(vm_op_arg0, ARG0, len)
vm_op_fn(vm_op_arg1, ARG1, len)
vm_op_fn(vm_op_arg2, ARG2, len)
vm_op_fn(vm_op_arg3, ARG3, len)

vm_op_fn(vm_op_not, NOT, nil)
vm_op_fn(vm_op_and, AND, nil)
vm_op_fn(vm_op_xor, XOR, nil)
vm_op_fn(vm_op_or, OR, nil)
vm_op_fn(vm_op_bnot, BNOT, nil)
vm_op_fn(vm_op_band, BAND, nil)
vm_op_fn(vm_op_bxor, BXOR, nil)
vm_op_fn(vm_op_bor, BOR, nil)
vm_op_fn(vm_op_bsl, BSL, nil)
vm_op_fn(vm_op_bsr, BSR, nil)

vm_op_fn(vm_op_neg, NEG, nil)
vm_op_fn(vm_op_add, ADD, nil)
vm_op_fn(vm_op_sub, SUB, nil)
vm_op_fn(vm_op_mul, MUL, nil)
vm_op_fn(vm_op_lmul, LMUL, nil)
vm_op_fn(vm_op_div, DIV, nil)
vm_op_fn(vm_op_rem, REM, nil)

vm_op_fn(vm_op_eq, EQ, nil)
vm_op_fn(vm_op_ne, NE, nil)
vm_op_fn(vm_op_gt, GT, nil)
vm_op_fn(vm_op_ge, GE, nil)
vm_op_fn(vm_op_lt, LT, nil)
vm_op_fn(vm_op_le, LE, nil)
vm_op_fn(vm_op_cmp, CMP, nil)

vm_op_fn(vm_op_ret, RET, nil)
vm_op_fn(vm_op_call, CALL, mod)
vm_op_fn(vm_op_load, LOAD, nil)
vm_op_fn(vm_op_jmp, JMP, off)
vm_op_fn(vm_op_jz, JZ, off)
vm_op_fn(vm_op_jnz, JNZ, off)

vm_op_fn(vm_op_reset, RESET, nil)
vm_op_fn(vm_op_yield, YIELD, nil)
vm_op_fn(vm_op_tsc, TSC, nil)
vm_op_fn(vm_op_fault, FAULT, nil)

vm_op_fn(vm_op_io, IO, len)
vm_op_fn(vm_op_ios, IOS, nil)

vm_op_fn(vm_op_pack, PACK, nil)
vm_op_fn(vm_op_unpack, UNPACK, nil)

#undef vm_op_fn
