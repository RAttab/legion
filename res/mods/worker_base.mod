# -*- mode:text -*-

# Basic worker implementation

# Read our assigned printer and save it $4
push !io_recv
push 1
ios
popr $4

# Start by mining element a
push !ele_a
popr $1


# Start of our loop
start:

# Keep track of how much of our element we have
push 0
push !io_cargo
io 2
pop
popr $2

# harvest our current element
pushr $1
push !io_harvest
io 2

# check our cargo
push 1
push !io_cargo
io 2

# keep only the count and make a copy
pop
dupe
popr $3

# Check if our cargo is full
push 255
eq
jnz @worker-full

# Check if we've harvested nothing
pushr $3
pushr $2
eq
jnz @done-ele

# Harvest more
jmp @start

# We're done with our current element;
# move to the next one
done-ele:
pushr $1
push 1
add
popr $1

# dock with our assigned printer
worker-full:
pushr $4
push !io_dock
io 2

# dump our cargo
push 1
push 0
pack
push !io_put
io 2

# undock from printer
push !io_undock
io 1

# keep on trucking
jmp @start
