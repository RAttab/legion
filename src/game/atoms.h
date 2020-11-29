/* atoms.h
   Rémi Attab (remi.attab@gmail.com), 23 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// atoms_io
// -----------------------------------------------------------------------------

enum atom_io
{
    ATOM_IO_MIN = 0x80000000,

    io_noop,
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
    
    ATOM_IO_MAX,
    ATOM_IO_CAP = 0x8FFFFFFF,
};
