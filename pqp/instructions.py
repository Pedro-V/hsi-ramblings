regs = [
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp",
    "ebp", 'r8d', 'r9d', 'r10d', 'r11d', "r12d", "r13d", "r14d",
    "r15d",
]

dois_regs = ["mov", "add", "sub", "and", "or", "xor", "cmp"]
um_inteiro = ["jmp", "jg", "jl", "je"]
reg_inteiro = ["mov", "shl", "shr"]

meu_inteiro = 0b1010 # 4 bits
covered_mem = False

for inst in dois_regs:
    for reg1 in regs:
        for reg2 in regs:
            if inst == 'mov' and not covered_mem:
                print(f'{inst} [{reg1}], {reg2}')
                print(f'{inst} {reg1}, [{reg2}]')
            else:
                print(f'{inst} {reg1}, {reg2}')
    covered_mem = True

for inst in um_inteiro:
    print(f'{inst} {hex(meu_inteiro)}')

for inst in reg_inteiro:
    for reg in regs:
        print(f'{inst} {reg}, {hex(meu_inteiro)}')