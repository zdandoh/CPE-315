   .global intsub
intsub:
   push {r1, r2, lr}
   // change the sign of the second arg
   mvn r1, r1
   mov r2, r0
   mov r0, #1
   bl intadd

   mov r1, r2
   bl intadd

   pop {r1, r2, pc}
