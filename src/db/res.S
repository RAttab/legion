	/* db.S
	   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
	   FreeBSD-style copyright and disclaimer apply
	*/


	// ---------------------------------------------------------------------
	// DB_RES
	// ---------------------------------------------------------------------

	// Includes a file into the executable under the symbols
	// <name>_data and <name>_len
	.macro DB_RES name:req, file:req

	.global \name\()_data
	.type \name\()_data, STT_OBJECT
	.balign 64
\name\()_data:
	.incbin "\file"
\name\()_data_end:

	.global \name\()_len
	.type \name\()_len, STT_OBJECT
	.balign 4
\name\()_len:
	.4byte \name\()_data_end - \name\()_data

	.endm


	// ---------------------------------------------------------------------
	// data
	// ---------------------------------------------------------------------

	.section .rodata

	.include "db/gen/man.S"

	DB_RES db_img_cursor "./res/img/cursor.bmp"
	DB_RES db_img_map "./res/img/map.bmp"

        DB_RES db_font_regular "./res/font/IbmPlexMonoRegular.otf"
        DB_RES db_font_italic "./res/font/IbmPlexMonoItalic.otf"
        DB_RES db_font_bold "./res/font/IbmPlexMonoBold.otf"


	// ---------------------------------------------------------------------
	// engine
	// ---------------------------------------------------------------------

        DB_RES db_shader_prim_vert "./res/shaders/prim.vert"
        DB_RES db_shader_prim_frag "./res/shaders/prim.frag"
        DB_RES db_shader_tex_vert "./res/shaders/tex.vert"
        DB_RES db_shader_tex_frag "./res/shaders/tex.frag"
        DB_RES db_shader_font_vert "./res/shaders/font.vert"
        DB_RES db_shader_font_frag "./res/shaders/font.frag"

	DB_RES db_texture_cursor "./res/textures/cursor.tga"
	DB_RES db_texture_star "./res/textures/star.tga"
