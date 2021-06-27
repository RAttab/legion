# -*- mode:text -*-

push 0xD101
push 0xD2000001
push !io_prog
pack
io 2

push 0xD103
push 0xD2000002
push !io_prog
pack
io 2

push 0xD1D0
push 0xD1000001
push !io_prog
pack
io 2

push 0xD3D0
push 0xD3000001
push !io_prog
pack
io 2

inf:
yield
jmp @inf
