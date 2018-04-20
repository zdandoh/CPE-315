   .global intadd
intadd:
   push {r1, r2, r3, lr}
start:
   and r3, r0, r1
   eor r0, r0, r1
   mov r1, r3, LSL #1

   cmp r1, #0
   bne start
   pop {r1, r2, r3, pc}
