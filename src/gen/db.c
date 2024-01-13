/* gen.c
   Rémi Attab (remi.attab@gmail.com), 18 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

struct symbol symbol_to_enum(struct symbol sym)
{
    for (size_t i = 0; i < sym.len; ++i)
        if (sym.c[i] == '-') sym.c[i] = '_';
    return sym;
}


// -----------------------------------------------------------------------------
// implementation
// -----------------------------------------------------------------------------

#include "db_types.c"
#include "db_parse.c"
#include "db_stars.c"
#include "db_man.c"
#include "db_gen.c"


// -----------------------------------------------------------------------------
// run
// -----------------------------------------------------------------------------

bool db_run(const char *res, const char *src)
{
    struct db_state state = {0};
    snprintf(state.path.in, sizeof(state.path.in), "%s/tech.lisp", src);
    snprintf(state.path.io, sizeof(state.path.io), "%s/io.lisp", res);
    snprintf(state.path.out, sizeof(state.path.out), "%s", src);
    snprintf(state.path.stars, sizeof(state.path.stars), "%s/stars", res);
    snprintf(state.path.man, sizeof(state.path.out), "%s/man", res);

    state.info = vec_info_reserve(255);

    {
        db_file_open(&state.files.im_enum, state.path.out, "item.h");
        db_file_write(&state.files.im_enum,
                "enum item : uint8_t\n{\n  item_nil = 0x00,\n");

        db_file_open(&state.files.im_register, state.path.out, "im_register.h");

        db_file_open(&state.files.im_includes, state.path.out, "im_includes.h");
        db_file_open(&state.files.im_control, state.path.out, "im_control.h");
        db_file_open(&state.files.im_factory, state.path.out, "im_factory.h");

        db_file_open(&state.files.specs_enum, state.path.out, "specs_enum.h");
        db_file_open(&state.files.specs_register, state.path.out, "specs_register.h");
        db_file_open(&state.files.specs_value, state.path.out, "specs_value.h");

        db_file_open(&state.files.tapes, state.path.out, "tapes.h");
        db_file_open(&state.files.tapes_info, state.path.out, "tapes_info.h");

        db_file_open(&state.files.io_enum, state.path.out, "io_enum.h");
        db_file_open(&state.files.ioe_enum, state.path.out, "ioe_enum.h");
        db_file_open(&state.files.io_register, state.path.out, "io_register.h");

        db_file_open(&state.files.stars_prefix, state.path.out, "stars_prefix.h");
        db_file_open(&state.files.stars_suffix, state.path.out, "stars_suffix.h");
        db_file_open(&state.files.stars_rolls, state.path.out, "stars_rolls.h");

        db_file_open(&state.files.man, state.path.out, "man.S");
    }

    db_parse_atoms(&state, state.path.in);
    db_gen_items(&state);
    db_gen_specs_tapes(&state, state.path.in);
    db_gen_io(&state, state.path.io);
    db_gen_stars(&state);
    db_gen_man(&state);

    {
        db_file_write(&state.files.im_enum, "};\n");
        db_file_close(&state.files.im_enum);

        db_file_close(&state.files.im_register);

        db_file_close(&state.files.im_includes);
        db_file_close(&state.files.im_control);
        db_file_close(&state.files.im_factory);

        db_file_close(&state.files.specs_value);
        db_file_close(&state.files.specs_enum);
        db_file_close(&state.files.specs_register);

        db_file_close(&state.files.tapes);
        db_file_close(&state.files.tapes_info);

        db_file_close(&state.files.io_enum);
        db_file_close(&state.files.ioe_enum);
        db_file_close(&state.files.io_register);

        db_file_close(&state.files.stars_prefix);
        db_file_close(&state.files.stars_suffix);
        db_file_close(&state.files.stars_rolls);

        db_file_close(&state.files.man);
    }

    htable_reset(&state.atoms.name);
    vec_info_free(state.info);
    return true;
}
