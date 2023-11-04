/* sys.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// sys
// -----------------------------------------------------------------------------

static char sys_mods_path[PATH_MAX] = {0};

static void sys_populate_impl(void)
{
    im_populate();
    mod_compiler_init();
    specs_populate();
    tapes_populate();
    stars_populate();
    ast_populate();

    {
        struct atoms *atoms = atoms_new();

        im_populate_atoms(atoms);
        io_populate_atoms(atoms);
        man_populate(atoms);

        atoms_free(atoms);
    }
}

void sys_populate(void)
{
    snprintf(sys_mods_path, sizeof(sys_mods_path),
            "%s/.legion", path_home());

    int ret = mkdir(sys_mods_path, 0740);
    if (ret && errno != EEXIST)
        failf_errno("unable to create mods dir '%s'", sys_mods_path);

    sys_populate_impl();
}

void sys_populate_tests(void)
{
    snprintf(sys_mods_path, sizeof(sys_mods_path), "./res/mods");

    sys_populate_impl();
}

void sys_path_res(const char *name, char *dst, size_t len)
{
    snprintf(dst, len, "./res/%s", name);
}

void sys_path_mods(char *dst, size_t len)
{
    snprintf(dst, len, "%s", sys_mods_path);
}

void sys_path_mod(const char *name, char *dst, size_t len)
{
    snprintf(dst, len, "%s/%s.lisp", sys_mods_path, name);
}
