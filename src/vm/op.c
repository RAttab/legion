/* op.c
   Rémi Attab (remi.attab@gmail.com), 02 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "op.h"
#include "utils/str.h"

// -----------------------------------------------------------------------------
// vm_op
// -----------------------------------------------------------------------------

const char *vm_op_str(enum vm_op type)
{
    switch (type)
    {

#define vm_op_fn(op, str, arg) \
    case op: { return #str; }
#include "opx.h"

    default: { return "???"; }

    }
}

enum vm_op_arg vm_op_arg(enum vm_op type)
{
    switch (type)
    {

#define vm_op_fn(op, str, arg) \
    case op: { return vm_op_arg_ ## arg; }
#include "opx.h"

    default: { assert(false); }

    }
}

size_t vm_op_arg_bytes(enum vm_op_arg type)
{
    switch (type)
    {
    case vm_op_arg_nil: { return 0; }
    case vm_op_arg_reg: { return sizeof(vm_reg); }
    case vm_op_arg_lit: { return sizeof(vm_word); }
    case vm_op_arg_mod: { return sizeof(vm_word); }
    case vm_op_arg_off: { return sizeof(vm_ip); }
    case vm_op_arg_len: { return sizeof(uint8_t); }
    default: { assert(false); }
    }
}

size_t vm_op_arg_fmt(enum vm_op_arg type, vm_word val, char *dst, size_t len)
{
    const char *const first = dst;
    const char *const last = dst + len;

    switch (type)
    {
    case vm_op_arg_nil: { break; }

    case vm_op_arg_reg: {
        assert(len >= 1);
        *dst = '$'; dst++;
        dst += str_utox(val, dst, last - dst);
        break;
    }

    case vm_op_arg_len:
    case vm_op_arg_off:
    case vm_op_arg_lit:
    case vm_op_arg_mod: {
        assert(len >= 2);
        *dst = '0'; dst++;
        *dst = 'x'; dst++;
        dst += str_utox(val, dst, last - dst);
        break;
    }

    default: { assert(false); }
    }

    return dst - first;
}
