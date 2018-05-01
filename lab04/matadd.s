    /* This function has 5 parameters, and the declaration in the
       C-language would look like:

       void matadd (int **C, int **A, int **B, int height, int width)

       C, A, B, and height will be passed in r0-r3, respectively, and
       width will be passed on the stack.

       You need to write a function that computes the sum C = A + B.

       A, B, and C are arrays of arrays (matrices), so for all elements,
       C[i][j] = A[i][j] + B[i][j]

       You should start with two for-loops that iterate over the height and
       width of the matrices, load each element from A and B, compute the
       sum, then store the result to the correct element of C. 

       This function will be called multiple times by the driver, 
       so don't modify matrices A or B in your implementation.

       As usual, your function must obey correct ARM register usage
       and calling conventions. */

	.arch armv7-a
	.text
	.align	2
	.global	matadd
	.syntax unified
	.arm
matadd:
   push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, lr}
   ldr r4, [sp, #48]

   mov r10, #4
   mul r3, r3, r10
   mul r4, r4, r10

   mov r5, #0 // y
   mov r6, #0 // x

loop:
   ldr r7, [r1, r5]
   ldr r7, [r7, r6]

   ldr r8, [r2, r5]
   ldr r8, [r8, r6]

   add r7, r7, r8 // the sum

   ldr r8, [r0, r5]
   str r7, [r8, r6]

   add r6, r6, #4

   // Check whether to increment y
   cmp r6, r4
   bge incry
   b endincr
incry:
   add r5, r5, #4
   mov r6, #0

endincr:
   cmp r5, r3
   blt loop

   pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, pc}

printtwo:
   .asciz "Width: %d, height: %d\n\000"
printnum:
   .asciz "%d\n\000"
printbr:
   .asciz "STOP\n\000"
