/* op_xmacro.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2021
   FreeBSD-style copyright and disclaimer apply

xmacro header:
  op_fn(op, arg) -> op_fn(OP_ ## op, nil | lit | reg | off | mod)
*/

#ifndef op_fn
#  error "op_fn must be declared when including op_xmacro.h"
#endif

op_fn(NOOP, nil)

op_fn(PUSH, lit)
op_fn(PUSHR, reg)
op_fn(PUSHF, nil)
op_fn(POP, nil)
op_fn(POPR, reg)
op_fn(DUPE, nil)
op_fn(SWAP, nil)
op_fn(ARG0, len)
op_fn(ARG1, len)
op_fn(ARG2, len)
op_fn(ARG3, len)

op_fn(NOT, nil)
op_fn(AND, nil)
op_fn(XOR, nil)
op_fn(OR, nil)
op_fn(BNOT, nil)
op_fn(BAND, nil)
op_fn(BXOR, nil)
op_fn(BOR, nil)
op_fn(BSL, nil)
op_fn(BSR, nil)

op_fn(NEG, nil)
op_fn(ADD, nil)
op_fn(SUB, nil)
op_fn(MUL, nil)
op_fn(LMUL, nil)
op_fn(DIV, nil)
op_fn(REM, nil)

op_fn(EQ, nil)
op_fn(NE, nil)
op_fn(GT, nil)
op_fn(GE, nil)
op_fn(LT, nil)
op_fn(LE, nil)
op_fn(CMP, nil)

op_fn(RET, nil)
op_fn(CALL, mod)
op_fn(LOAD, nil)
op_fn(JMP, off)
op_fn(JZ, off)
op_fn(JNZ, off)

op_fn(RESET, nil)
op_fn(YIELD, nil)
op_fn(TSC, nil)
op_fn(FAULT, nil)

op_fn(IO, len)
op_fn(IOS, nil)
op_fn(IOR, reg)

op_fn(PACK, nil)
op_fn(UNPACK, nil)

#undef op_fn
