   .syntax unified
   .arch armv6
   .fpu vfp

   .global main

main:
   push {ip, lr}
repeat:
   // Print prompt 1
   ldr r0, =num1
   bl printf

   // Do scanf for first operand
   ldr r0, =scanno
   mov r1, sp
   bl scanf
   ldr r4, [sp] // save first value into r4

   ldr r0, =num2
   bl printf

   ldr r0, =scanno
   mov r1, sp
   bl scanf
   ldr r5, [sp] // save second value into r5

   ldr r0, =operation
   bl printf

   ldr r0, =scanchar
   mov r1, sp
   bl scanf
   ldrb r3, [sp]

   mov r0, r4
   mov r1, r5

   cmp r3, #43
   beq add
   cmp r3, #45
   beq sub
   cmp r3, #42
   beq mult

   ldr r0, =invalidop //Invalid operation
   bl printf

again:
   ldr r0, =againprompt
   bl printf
   
   ldr r0, =scanchar
   mov r1, sp
   bl scanf
   ldrb r0, [sp]

   cmp r0, #121
   beq repeat

   pop {ip, pc}

add:
   bl intadd
   
   mov r1, r0
   ldr r0, =resultis
   bl printf

   b again
sub:
   bl intsub

   mov r1, r0
   ldr r0, =resultis
   bl printf

   b again
mult:
   bl intmul

   mov r1, r0
   ldr r0, =resultis
   bl printf

   b again

ain:
   .asciz "Adding\n\000"
sin:
   .asciz "Subbing\n\000"
multin:
   .asciz "Multing\n\000"
yes:
   .byte 'y'
no:
   .byte 'n'
scanchar:
   .asciz " %c"
scanno:
   .asciz " %d"
printnum:
   .asciz "\n%d\n\000"
num1:
   .asciz "Enter Number 1: \000"
num2:
   .asciz "Enter Number 2: \000"
operation:
   .asciz "Enter Operation: \000"
invalidop:
   .asciz "Invalid\n\000"
againprompt:
   .asciz "Again? \000"
resultis:
   .asciz "Result is: %d\n\000"
