   .global intmul
intmul:
   push {r1, r2, r3, lr}

   mov r2, r1 // num1
   mov r1, r0 // num2
   mov r0, #0 // product

loop:
   cmp r2, #0
   beq end
   
   and r3, r2, #1
   cmp r3, #0
   beq skip
   bl intadd

skip:
   mov r1, r1, LSL #1
   mov r2, r2, LSR #1
   b loop

end:
   pop {r1, r2, r3, pc}
