/* sys.h
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// sys
// -----------------------------------------------------------------------------

void sys_populate(void);
void sys_path_res(const char *name, char *dst, size_t len);
