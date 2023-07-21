	/* db.S
	   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
	   FreeBSD-style copyright and disclaimer apply
	*/

	// Includes a file into the executable under the symbols
	// <name>_data and <name>_len
	.macro DB_BIN name:req, file:req

	.global \name\()_data
	.type \name\()_data, STT_OBJECT
	.balign 64
\name\()_data:
	.incbin "\file"
\name\()_data_end:

	.global \name\()_len
	.type \name\()_len, STT_OBJECT
	.balign 8
\name\()_len:
	.4byte \name\()_data_end - \name\()_data

	.endm
