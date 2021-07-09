# -*- mode:text -*-

push !item_elem_a
push !item_extract_1
push 24
bsl
push 1
add
push !io_prog
pack
io 2

push !item_elem_f
push !item_extract_1
push 24
bsl
push 2
add
push !io_prog
pack
io 2

push !item_frame
push !item_printer_1
push 24
bsl
push 1
add
push !io_prog
pack
io 2

push !item_circuit
push !item_printer_1
push 24
bsl
push 2
add
push !io_prog
pack
io 2

push !item_extract_1
push !item_assembly_1
push 24
bsl
push 1
add
push !io_prog
pack
io 2

push !item_extract_1
push !item_deploy
push 24
bsl
push 1
add
push !io_item
pack
io 2

inf:
yield
jmp @inf
