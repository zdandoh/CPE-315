   .arch armv6
   .fpu vfp
   .text

@ print function is complete, no modifications needed
    .global print
print:
   stmfd sp!, {r3, lr}
   mov   r3, r0
   mov   r2, r1
   ldr   r0, startstring
   mov   r1, r3
   bl printf
   ldmfd sp!, {r3, pc}

startstring:
   .word string0

    .global towers
towers:

   push {r0, r1, r2, r3, r4, r5, r6, lr}

if:
   cmp r0, #2
   bge else

   /* I push and pop frequently around functions in this program
      this isn't because I don't understand the side-effects
      of calling functions, but because it's easier than
      swapping a bunch of registers around all the time
      and then restoring the state of a previous register from another register.
   */

   push {r0, r1}
   mov r0, r1
   mov r1, r2
   bl print
   pop {r0, r1}
   mov r0, #1
   b endif

else:
   mov r4, #6
   sub r4, r4, r1
   sub r4, r4, r2

   push {r0, r1, r2}
   sub r0, r0, #1
   mov r2, r4
   bl towers
   mov r5, r0
   pop {r0, r1, r2}

   push {r0}
   mov r0, #1
   bl towers
   add r5, r5, r0
   pop {r0}

   push {r0, r1}
   sub r0, r0, #1
   mov r1, r4
   bl towers

   add r5, r5, r0
   pop {r0, r1}

   mov r0, r5

endif:
   /* Restore Registers */
   mov r8, r0
   pop {r0, r1, r2, r3, r4, r5, r6, lr}
   mov r0, r8
   mov pc, lr

@ Function main is complete, no modifications needed
    .global main
main:
   str   lr, [sp, #-4]!
   sub   sp, sp, #20
   ldr   r0, printdata
   bl printf
   ldr   r0, printdata+4
   add   r1, sp, #12
   bl scanf
   ldr   r0, [sp, #12]
   mov   r1, #1
   mov   r2, #3
   bl towers
   str   r0, [sp]
   ldr   r0, printdata+8
   ldr   r1, [sp, #12]
   mov   r2, #1
   mov   r3, #3
   bl printf
   mov   r0, #0
   add   sp, sp, #20
   ldr   pc, [sp], #4
end:

printdata:
   .word string1
   .word string2
   .word string3

string0:
   .asciz   "Move from peg %d to peg %d\n"
string1:
   .asciz   "Enter number of discs to be moved: "
string2:
   .asciz   "%d"
   .space   1
string3:
   .ascii   "\n%d discs moved from peg %d to peg %d in %d steps."
   .ascii   "\012\000"
