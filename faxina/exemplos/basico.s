# esse código é alimentado ao Defuse para obter codigo de maquina
# que será o exemplo dado ao programa
.section .text
    .globl main

main:
    push %rbp
    movq %rsp, %rbp
    movq $1, %rdi        # Initialize rdi with 1 (starting number)

loop_start:
    # Print the value in rdi
    movq %rdi, %rax      # Copy rdi to rax
    addq $'0', %rax      # Convert to ASCII
    pushq %rax           # Push the ASCII character onto the stack
    movq $1, %rax        # syscall number for write
    movl $1, %edi        # write to stdout (int fd=1)
    movq %rsp, %rsi      # use char on stack
    movq $1, %rdx        # size_t len = 1 char to write.
    syscall              # call the kernel, it looks at registers to decide what to do

    # Print a newline character
    pushq $'\n'          # push qword 0xA (ASCII '\n')
    movq $1, %rax        # syscall number for write
    movl $1, %edi        # write to stdout (int fd=1)
    movq %rsp, %rsi      # use char on stack
    movq $1, %rdx        # size_t len = 1 char to write.
    syscall              # call the kernel, it looks at registers to decide what to do

    # Increment rdi (the value to print)
    incq %rdi

    # Check if we've reached 101 (after printing 100)
    cmpq $101, %rdi
    je loop_end

    # Continue the loop
    jmp loop_start

loop_end:
    leave
    ret
