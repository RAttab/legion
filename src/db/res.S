	/* db.S
	   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
	   FreeBSD-style copyright and disclaimer apply
	*/


	// ---------------------------------------------------------------------
	// DB_INCLUDE_RES
	// ---------------------------------------------------------------------

	// Includes a file into the executable under the symbols
	// <name>_data and <name>_len
	.macro DB_INCLUDE_RES name:req, file:req

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

	DB_INCLUDE_RES db_img_cursor "./res/img/cursor.bmp"
	DB_INCLUDE_RES db_img_map "./res/img/map.bmp"

        DB_INCLUDE_RES db_font_regular "./res/font/IbmPlexMonoRegular.otf"
        DB_INCLUDE_RES db_font_italic "./res/font/IbmPlexMonoItalic.otf"
        DB_INCLUDE_RES db_font_bold "./res/font/IbmPlexMonoBold.otf"
