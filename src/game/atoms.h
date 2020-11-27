/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

enum atoms_io
{
    ATOM_IO_MAX = 0x8FFFFFFF,
    ATOM_IO_MIN = 0x80000000,

    io_noop = ATOM_IO_MIN,
    io_ok,
    io_fail,

    // common
    io_id,
    io_target,
    io_send,
    io_sendn,
    io_recv,
    io_recvn,
    io_cargo,
    io_vent,

    //worker
    io_dock,
    io_undock,
    io_take,
    io_put,
    io_harvest,
};

const char *atoms_io_str(enum atoms_io);
