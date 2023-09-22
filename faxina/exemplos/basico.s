# esse código é alimentado ao Defuse para obter codigo de maquina
# que será o exemplo dado ao programa
.intel_syntax noprefix

.section .text
    .globl main
main:
    push   rbp
    mov    rbp,rsp
    mov    r10,0x1

loop:
    mov    rax,r10
    add    al,0x30
    push   rax
    mov    rax,0x1
    mov    edi,0x1
    mov    rsi,rsp
    mov    rdx,0x1
    syscall
    push   0xa
    mov    rax,0x1
    mov    edi,0x1
    mov    rsi,rsp
    mov    rdx,0x1
    syscall
    inc    r10
    cmp    r10,0xa
    jl     loop
end:
    leave
    ret 
