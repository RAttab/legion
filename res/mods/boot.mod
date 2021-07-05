# -*- mode:text -*-

push 0xD001
push 0xD0000001
push !io_prog
pack
io 2

push 0xD003
push 0xD0000002
push !io_prog
pack
io 2

push 0xD1A0
push 0xD1000001
push !io_prog
pack
io 2

push 0xD2A0
push 0xD2000001
push !io_prog
pack
io 2

inf:
yield
jmp @inf
