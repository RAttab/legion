/* sys.h
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// sys
// -----------------------------------------------------------------------------

void sys_populate(void);
void sys_populate_tests(void);
void sys_path_res(const char *name, char *dst, size_t len);

void sys_path_mods(char *dst, size_t len);
void sys_path_mod(const char *name, char *dst, size_t len);
