.intel_syntax noprefix

.section .bss
    .lcomm i, 4
    .lcomm input, 8
    .lcomm output, 8

.section .rodata
    error_read_message: .string "Nao foi possivel abrir o arquivo de input\n"
    unsigned_int_fmt:   .string "%u\n"
    signed_int_fmt:     .string "%d\n"
    index_fmt:          .string "[%u] "
    element_fmt:        .string "%d%c"
    write_permission:   .string "w"
    read_permission:    .string "r"
    
.section .text
    .global main
    .extern printf
    .extern fopen
    .extern fclose
    .extern malloc
    .extern scanf
    .extern qsort

main:
    push rbp            # prologue
    mov rbp, rsp
    # read input file
    sub rsp, 16
    mov [rbp - 8], rsi  # argv
    mov rdi, [rbp - 8]
    add rdi, 8          # argv + 8 (first file)
    mov rdi, [rdi]      # dereference it
    lea rsi, [rip + read_permission]
    call fopen
    test rax, rax       # check if reading went worng (rax == 0)
    je error_read
    mov [rip + input], rax # if read went ok, store the FILE pointer
    mov rdi, [rbp - 8]  # similar process for output file
    add rdi, 16
    mov rdi, [rdi]
    lea rsi, [rip + write_permission]
    call fopen
    mov [rip + output], rax
    # read array amount
    mov rdi, [rip + input]
    lea rsi, [rip + unsigned_int_fmt]
    lea rdx, [rbp - 4]      # array_amount
    call fscanf
    loop_processing:    # process each input array
        mov eax, [rbp - 4]
        cmp [rip + i], eax
        jae finish_main # if i >= array_amount
        call process_array
        add dword ptr [rip + i], 1
        jmp loop_processing
    finish_main:        # close files and exit
        mov rdi, [rip + input]
        call fclose
        mov rdi, [rip + output]
        call fclose
        mov eax, 0
        leave
        ret
    error_read:         # if reading went wrong, inform and exit
        mov rdi, [rip + stdout]
        lea rsi, [rip + error_read_message]
        call fprintf
        mov rdi, 1      # non-null exit code
        call exit

process_array:          # read, sort and print current array
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 16], 0   # array
    mov dword ptr [rbp - 8], 0    # len
    read_array:
        mov rdi, [rip + input]
        lea rsi, [rip + unsigned_int_fmt]
        lea rdx, [rbp - 8]
        call fscanf

        mov rdi, 0
        mov edi, [rbp - 8]
        shl edi, 2              # allocates len * 4 bytes
        call malloc
        mov [rbp - 16], rax
    mov dword ptr [rbp - 4], 0  # counter
    read_elements:
        mov eax, [rbp - 4]
        cmp eax, [rbp - 8]
        jae sort_array
        mov rdi, [rip + input]
        lea rsi, [rip + signed_int_fmt]
        lea rdx, [rax * 4 + 0]
        add rdx , [rbp - 16]
        inc eax
        mov [rbp - 4], eax  # save counter before function call
        call fscanf
        jmp read_elements
    sort_array:
        mov rdi, [rbp - 16]     # array
        mov esi, [rbp - 8]      # len
        mov edx, 4              # array elements' size (4 byte integers)
        lea rcx, [rip + compare]# comparison function
        call qsort
    print_index:
        mov rdi, [rip + output] 
        lea rsi, [rip + index_fmt]
        mov edx, [rip + i]
        call fprintf
    mov dword ptr [rbp - 4], 0    # counter
    print_elements:
        mov ecx, [rbp - 4]
        cmp ecx, [rbp - 8]
        jae finish_process
        mov rdi, [rip + output]
        lea rsi, [rip + element_fmt]
        lea rdx, [ecx * 4 + 0]
        add rdx, [rbp - 16]
        mov edx, [rdx]  # current element
        mov eax, [rbp - 8]
        sub eax, 1
        cmp ecx, eax    # checks if last iteration
        je last_iter    # if it is, next char is '\n'
        mov rax, 0x20   # else, next char is ' '
        jmp print
        last_iter:
            mov rax, 0xa
        print:
            inc ecx
            mov [rbp - 4], ecx  # saves counter
            mov ecx, eax        # sets last character
            call fprintf
        jmp print_elements
    finish_process: # frees memory and leaves
        mov rdi, [rbp - 16]
        call free
        leave
        ret
 
 compare: # usual comparison function
    push rbp
    mov rbp, rsp
    mov eax, [rdi] 
    mov edx, [rsi]
    sub eax, edx
    leave
    ret
