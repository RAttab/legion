/* printer.c
   Rémi Attab (remi.attab@gmail.com), 27 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/obj.h"
#include "game/hunk.h"


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

enum
{
    printer_spec_io = 4, // * 8 = 32
    printer_spec_cargo = 4, // * 2 = 16
    printer_spec_docks = 2, // * 4 = 16
};

static_assert(s_cache_line ==
        printer_spec_io * sizeof(word_t) +
        printer_spec_cargo * sizeof(cargo_t) +
        print_spec_docks * sizeof(id_t));


struct obj *printer_alloc(struct hunk *hunk)
{
    return obj_alloc(hunk, ITEM_PRINTER, &(struct obj_spec) {
                .state = sizeof(struct printer),
                .stack = 1,
                .speed = 1,
                .io = printer_spec_io,
                .cargo = printer_spec_cargo,
                .docks = 2
            });
}

static void printer_pick(struct printer *state, struct obj *obj, word_t arg)
{
    if (!arg || arg > ITEM_MAX) return;
    state->pick = arg;
}

static void printer_move(struct printer *state, struct obj *obj, word_t arg)
{
    uint32_t x = 0, y = 0;
    vm_unpack(arg, &x, &y);

    if (x > 3 || y > 3) return;
    state->x = x;
    state->y = y;
}

static void printer_print(struct printer *state, struct obj *obj)
{
    uint8_t x = state->x;
    uint8_t y = state->y;

    uint8_t z = 0;
    while (z < 3 && state->matrix[x][y][z]) z++;
    if (z == 3) return;

    cargo_t *it = obj_cargo(obj);
    cargo_t *end = it + obj->cargos;
    do {
        if (cargo_item(*it) == state->pick) {
            *it = cargo_sub(*it, 1);
            break;
        }
    } while(++it < end);
    if (it == end) return;

    state->matrix[x][y][z] = obj->pick;
}

static void printer_program(struct printer *state, struct obj *obj, word_t arg)
{
    mod_t mod = arg;
    if (mod != arg) return;

    state->mod = mod;
}

static word_t printer_output(
        struct printer *state, struct obj *obj, struct hunk *hunk, word_t arg)
{

}

bool printer_io(
        struct obj *obj, struct hunk *hunk, void *state_, int64_t *buf, size_t len)
{
    struct printer *state = state_;
    struct vm *vm = obj_vm(obj);

    switch (buf[0]) {

    case IO_PICK: {
        if (!vm_io_check(vm, len, 2)) return true;
        printer_pick(state, obj, buf[1]);
        return true;
    }

    case IO_MOVE: {
        if (!vm_io_check(vm, len, 2)) return true;
        printer_move(state, obj, buf[1]);
        return true;
    }

    case IO_PRINT: {
        if (!vm_io_check(vm, len, 1)) return true;
        printer_print(state, obj);
        return true;
    }

    case IO_PROGRAM: {
        if (!vm_io_check(vm, len, 2)) return true;
        printer_program(state, program, arg[1]);
        return true;
    }

    case IO_OUTPUT: {
        if (!vm_io_check(vm, len, 2)) return true;
        buf[0] = printer_print(state, obj, hunk, arg[1]);
        vm_io_write(vm, 1, buf);
        return true;
    }

    default: { return false };
    }
}
