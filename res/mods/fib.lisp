(asm
 (PUSH 1)
 (PUSH 1)
 (POPR $0)

 (@ loop)

 (PUSHR $0)
 (SWAP)
 (POPR $0)
 (PUSHR $0)
 (ADD)

 (YIELD)
 (JMP loop))
