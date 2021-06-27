# -*- mode:text -*-
push 1
push 1
popr $1

loop:

pushr $1
swap
popr $1
pushr $1
add

yield
jmp @loop
