#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#define MEM_SIZE 128
uint8_t MEM[MEM_SIZE];
#define N_REGS 16
int32_t regs[N_REGS];
#define N_TEMPLATES 16
// usage[i] => number of times i-th template has been executed
uint64_t template_usage[N_TEMPLATES];
#define PQP_PADDING 4
#define X86_PADDING 16
#define N_INSTRUCTIONS MEM_SIZE / PQP_PADDING
#define JMP_SCALAR X86_PADDING / PQP_PADDING  
#define EXIT_INSTRUCTION_INDEX N_INSTRUCTIONS + 1
#define EXITTER_FLAG -1
// holds translated bytes before each injection
int8_t code[X86_PADDING];
// pointer to last byte we wrote in code
int8_t p_code;
// current index of instruction
int16_t pc;
uint8_t exitted_normally;
// page where we will inject code and jit function
void *memory_page;
int32_t length;
int8_t (*jit)(void *, void *, void *);
// translation utilities
int32_t rx, ry, i16;
int8_t instruction;
uint8_t cond = 0b000;
// files
FILE *input, *output;

void emit_byte(int8_t b) {
    code[p_code] = b;
    p_code++;
}

// C standard asks for at least 1 parameter before elipsis
void emit_bytes(int n, ...) {
    va_list args;
    va_start(args, n);
    for (size_t i = 0; i < n; i++)
        emit_byte(va_arg(args, int));
    va_end(args);
}

void emit_5_bits(int32_t i16) {
    emit_byte(i16 & 0b11111);
}

void emit_32_bits(int32_t i32) {
    emit_byte(i32 & 0xff);
    emit_byte(i32 >> 8 & 0xff);
    emit_byte(i32 >> 16 & 0xffff);
    emit_byte(i32 >> 24 & 0xffff);
}

void emit_64_bits(int64_t i64) {
    emit_32_bits(i64 & 0xffffffff);
    emit_32_bits(i64 >> 32 & 0xffffffff);
}

void emit_nop(uint8_t nop_size) {
    switch (nop_size) {
        case 1:
            // nop
            emit_byte(0x90);
        case 2:
            // 66 nop
            emit_bytes(2, 0x66, 0x90);
            break;
        case 3:
            // nop DWORD ptr [eax]
            emit_bytes(3, 0x0f, 0x1f, 0x00);
            break;
        case 4:
            // nop DWORD PTR [eax + 0x00]
            emit_bytes(4, 0x0f, 0x1f, 0x40, 0x00);
            break;
        case 5:
            // nop DWORD PTR [eax + eax * 1 + 0x00]
            emit_bytes(5, 0x0f, 0x1f, 0x44, 0x00, 0x00);
            break;
        case 6:
            // 66 nop DWORD PTR [eax + eax* 1 + 0x00]
            emit_bytes(6, 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00);
            break;
        case 7:
            // nop DWORD PTR [eax + 0x00000000]
            emit_bytes(7, 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00);
            break;
        case 8:
            // nop DWORD PTR [eax + eax * 1 + 0x00000000]
            emit_bytes(8, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00);
            break;
        case 9:
            // 66 nop DWORD PTR [eax + eax * 1 + 0x00000000]
            emit_bytes(9, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00);
            break;
        case 10:
            emit_bytes(10, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00);
            break;
        case 11:
            emit_bytes(11, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00);
        default:
            break;
    }
}

#define DEFAULT_PAD_SIZE 11
void emit_padding() {
    uint8_t pad_size, to_pad = (X86_PADDING) - p_code;
    while (to_pad > 0) {
        pad_size = to_pad > DEFAULT_PAD_SIZE ? DEFAULT_PAD_SIZE : to_pad;
        emit_nop(pad_size);
        to_pad -= pad_size;
    }
}

// useful for moving regs addresses to real registers
void mov_both(uint8_t rx, uint8_t ry) {
    emit_bytes(3, 0x8b, 0x46, rx * 4);  // mov eax, DWORD PTR [rsi + rx * 4]
    emit_bytes(3, 0x8b, 0x4e, ry * 4);  // mov ecx, DWORD PTR [rsi + ry * 4]
}

void emit_prologue(void) {
    emit_byte(0x55);                   // push rbp
    emit_bytes(3, 0x48, 0x89, 0xe5);   // mov rbp, rsp
}

void emit_epilogue(void) {
    emit_byte(0x5d);    // pop rbp
    emit_byte(0xc3);    // ret
}

#define ADD 1
#define SUB 2
#define AND 3
#define OR 4
#define XOR 5
/*
 * helper for operations among registers
 */
void exec_op(uint8_t op, uint8_t rx, uint8_t ry) {
    emit_bytes(3, 0x8b, 0x4e, ry * 4);  // mov ecx, DWORD PTR [rsi + ry * 4]
    switch (op) {
        case ADD: emit_byte(0x01); break;
        case SUB: emit_byte(0x29); break;
        case AND: emit_byte(0x21); break;
        case  OR: emit_byte(0x09); break;
        case XOR: emit_byte(0x31); break;
        default: break;
    }
    emit_bytes(2, 0x4a, rx * 4);   // <op> DWORD PTR [rdx + rx * 4], ecx
}

void update_usage(uint8_t instruction) {
    emit_bytes(3, 0xff, 0x47, instruction * 4); // inc DWORD PTR [rdi + instruction * 4]
}

void empty_instruction(int8_t i) {
    if (i == EXITTER_FLAG) {
        // in case we reach the end without any illegal jump
        emit_bytes(2, 0x48, 0xb8);          // mov rax, &exitted_normally
        emit_64_bits((uint64_t)&exitted_normally);
        emit_bytes(3, 0xc6, 0x00, 0x01);    // mov BYTE PTR [rax], 1
    }
    emit_bytes(7, 0x48, 0xc7, 0xc0, i, 0x00, 0x00, 0x00);   // mov rax, i
    emit_epilogue();
}

// mov rx, i16
void template_0x0(uint8_t rx, uint8_t ry, int32_t i16) {
    emit_bytes(3, 0xc7, 0x42, rx * 4);  // mov DWORD PTR [rdx + rx * 4], i16
    emit_32_bits(i16);
}

// mov rx, ry
void template_0x1(uint8_t rx, uint8_t ry, int32_t i16) {
    mov_both(rx, ry);
    emit_bytes(3, 0x89, 0x4a, rx * 4); // mov DWORD PTR [rdx + rx * 4], ecx
}

// mov rx, [ry]
void template_0x2(uint8_t rx, uint8_t ry, int32_t i16) {
    mov_both(rx, ry);
    emit_bytes(3, 0x8b, 0x0c, 0x0e);   // mov ecx, DWORD PTR [rsi + rcx]
    emit_bytes(3, 0x89, 0x4a, rx * 4); // mov DWORD PTR [rdx + rx * 4], ecx
}

// mov [rx], ry
void template_0x3(uint8_t rx, uint8_t ry, int32_t i16) {
    mov_both(rx, ry);
    emit_bytes(3, 0x89, 0x0c, 0x02); // mov DWORD PTR [rsi + rax], ecx
}

// cmp rx, ry
void template_0x4(uint8_t rx, uint8_t ry, int32_t i16) {
    mov_both(rx, ry);
    emit_bytes(2, 0x39, 0xc8);       // cmp eax, ecx
    emit_byte(0x9f);                 // lahf
    emit_bytes(3, 0x49, 0x89, 0xc5); // mov r13, rax  
}

int8_t valid_jump(int32_t i16) {
    return (pc + PQP_PADDING + i16) < MEM_SIZE;
}

void emit_jump_addr(int32_t i16, uint8_t is_directioner_jmp) {
    int32_t offset;
    if (valid_jump(i16) || is_directioner_jmp)
        offset = i16;
    else
        // important parentheses for avoiding incorrect macro expansion
        offset = (EXIT_INSTRUCTION_INDEX) * (PQP_PADDING) - (pc + 4);
    emit_32_bits(offset * (JMP_SCALAR) - p_code - 4);
}

// jmp i16
void template_0x5(uint8_t rx, uint8_t ry, int32_t i16) {
    emit_byte(0xe9);         // jmp jump_addr
    emit_jump_addr(i16 + PQP_PADDING, 0);
}

void jump_cond(int32_t i16, int8_t cond_byte) {
    emit_bytes(3, 0x4c, 0x89, 0xe8); // mov rax, r13
    emit_byte(0x9e);                 // sahf
    emit_bytes(2, 0x0f, cond_byte);  // j[cond] jump_addr
    emit_jump_addr(i16 + PQP_PADDING, 0);
}

// jg i16
void template_0x6(uint8_t rx, uint8_t ry, int32_t i16) {
    jump_cond(i16, 0x8f);
}

// jl i16
void template_0x7(uint8_t rx, uint8_t ry, int32_t i16) {
    jump_cond(i16, 0x8c);
}

// je 16
void template_0x8(uint8_t rx, uint8_t ry, int32_t i16) {
    jump_cond(i16, 0x84);
}

// add rx, ry
void template_0x9(uint8_t rx, uint8_t ry, int32_t i16) {
    exec_op(ADD, rx, ry);
}

// sub rx, ry
void template_0xa(uint8_t rx, uint8_t ry, int32_t i16) {
    exec_op(SUB, rx, ry);
}

// and rx, ry
void template_0xb(uint8_t rx, uint8_t ry, int32_t i16) {
    exec_op(AND, rx, ry);
}

// or rx, ry
void template_0xc(uint8_t rx, uint8_t ry, int32_t i16) {
    exec_op(OR, rx, ry);
}

// xor rx, ry
void template_0xd(uint8_t rx, uint8_t ry, int32_t i16) {
    exec_op(XOR, rx, ry);
}

// sal rx, i16
void template_0xe(uint8_t rx, uint8_t ry, int32_t i16) {
    emit_bytes(4, 0xc1, 0x62, rx * 4, (uint8_t)i16);  // shl DWORD PTR [rdx + rx * 4], i16
}

// sar rx, i16
void template_0xf(uint8_t rx, uint8_t ry, int32_t i16) {
    emit_bytes(4, 0xc1, 0x7a, rx * 4, (uint8_t)i16); // sar DWORD PTR [rdx + rx * 4], i16
}

void (*templates[16])(uint8_t , uint8_t , int32_t) = {
    template_0x0, template_0x1, template_0x2, template_0x3, 
    template_0x4, template_0x5, template_0x6, template_0x7, 
    template_0x8, template_0x9, template_0xa, template_0xb,
    template_0xc, template_0xd, template_0xe, template_0xf
};

void parse(int16_t pc, int8_t *instruction, int32_t *rx, int32_t *ry, int32_t *i16) {
   *instruction = MEM[pc];
   *rx = MEM[pc + 1] >> 4;
   *ry = MEM[pc + 1] & 0x0f;
   *i16 = (int32_t)*(int16_t *)(&MEM[pc + 2]);
}

uint32_t print_addr(uint8_t cond_flag) {
    uint32_t printed_addr;
    if (cond == cond_flag) {
        pc += PQP_PADDING + i16;
        printed_addr = pc;
    }
    else {
        printed_addr = pc + PQP_PADDING + i16;
    }
    return printed_addr;
}

void translate(int8_t curr_line) {
    p_code = 0;
    pc = curr_line * 4;
    parse(pc, &instruction, &rx, &ry, &i16);
    fprintf(output, "0x%08X->", pc);
    switch (instruction) {
        case 0x0:
            fprintf(output, "MOV R%d=0x%08X\n", rx, i16);
            break;
        case 0x1:
            fprintf(output, "MOV R%d=R%d=0x%08X\n", rx, ry, regs[ry]);
            break;
        case 0x2:
            fprintf(
                output, 
                "MOV R%d=MEM[0x%02X,0x%02X,0x%02X,0x%02X]=[0x%02X,0x%02X,0x%02X,0x%02X]\n",
                rx,
                regs[ry],
                regs[ry] + 1,
                regs[ry] + 2,
                regs[ry] + 3,
                MEM[regs[ry]],
                MEM[regs[ry] + 1],
                MEM[regs[ry] + 2],
                MEM[regs[ry] + 3]
            );
            break;
        case 0x3:
            fprintf(
                output,
                "MOV MEM[0x%02X,0x%02X,0x%02X,0x%02X]=R%d=[0x%02X,0x%02X,0x%02X,0x%02X]\n",
                regs[rx],
                regs[rx] + 1,
                regs[rx] + 2,
                regs[rx] + 3,
                ry,
                regs[ry],
                regs[ry] >> 2,
                regs[ry] >> 4,
                regs[ry] >> 6
            );
            break;
        case 0x4:
            if (regs[rx] > regs[ry])
                cond = 0b100;
            else if (regs[rx] < regs[ry])
                cond = 0b010;
            else 
                cond = 0b001;
            fprintf(
                output,
                "CMP R%d<=>R%d(G=%d,L=%d,E=%d)\n",
                rx,
                ry,
                (cond & 0b100) >> 2,
                (cond & 0b010) >> 1,
                (cond & 0b001)
            );
            break;
        case 0x5:
            fprintf(
                output,
                "JMP 0x%08X\n",
                pc + i16 + PQP_PADDING
            );
            break;
        case 0x6:
            fprintf(
                output,
                "JG 0x%08X\n",
                pc + i16 + PQP_PADDING
            );
            break;
        case 0x7:
            fprintf(
                output,
                "JL 0x%08X\n",
                pc + i16 + PQP_PADDING
            );
            break;
        case 0x8:
            fprintf(
                output,
                "JE 0x%08X\n",
                print_addr(0b001)
            );
            break;
        case 0x9:
            fprintf(
                output,
                "ADD R%d+=R%d=0x%08X+0x%08X=0x%08X\n",
                rx,
                ry,
                regs[rx],
                regs[ry],
                regs[rx] + regs[ry]
            );
            break;
        case 0xa:
            fprintf(
                output,
                "SUB R%d-=R%d=0x%08X-0x%08X=0x%08X\n",
                rx,
                ry,
                regs[rx],
                regs[ry],
                regs[rx] - regs[ry]
            );
            break;
        case 0xb:
            fprintf(
                output,
                "AND R%d&=R%d=0x%08X&0x%08X=0x%08X\n",
                rx,
                ry,
                regs[rx],
                regs[ry],
                regs[rx] & regs[ry]
            );
            break;
        case 0xc:
            fprintf(
                output,
                "OR R%d|=R%d=0x%08X|0x%08X=0x%08X\n",
                rx,
                ry,
                regs[rx],
                regs[ry],
                regs[rx] | regs[ry]
            );
            break;
        case 0xd:
            fprintf(
                output,
                "XOR R%d^=R%d=0x%08X^0x%08X=0x%08X\n",
                rx,
                ry,
                regs[rx],
                regs[ry],
                regs[rx] ^ regs[ry]
            );
            break;
        case 0xe:
            i16 = (i16 >> 8) & 0b11111;
            fprintf(
                output,
                "SAL R%d<<=%d=0x%08X<<%d=0x%08X\n",
                rx,
                i16,
                regs[rx],
                i16,
                regs[rx] << i16
            );
            break;
        case 0xf:
            i16 = (i16 >> 8) & 0b11111;
            fprintf(
                output,
                "SAR R%d>>=%d=0x%08X>>%d=0x%08X\n",
                rx,
                i16,
                regs[rx],
                i16,
                regs[rx] >> i16
            );
            break;
        default:
            break;
    }
    update_usage(instruction);
    templates[instruction](rx, ry, i16);
    emit_padding();
}

void inject(int16_t i) {
    mprotect(memory_page, length, PROT_WRITE);
    memcpy(&((uint8_t *)memory_page)[(X86_PADDING)], (void *)code, X86_PADDING);
    mprotect(memory_page, length, PROT_EXEC);
}

// jit will always execute this first
// it returns back to the context where it was before last translation
void directioner(int16_t i) {
    emit_prologue();
    emit_byte(0xe9);
    emit_jump_addr(i * PQP_PADDING, 1);
    emit_padding();
}

void initialize_page (void) {
    memory_page = mmap(0, length, PROT_NONE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    for (int8_t i = 0; i < N_INSTRUCTIONS + 1; i++) {
        p_code = 0;
        if (i == N_INSTRUCTIONS)
            empty_instruction(EXITTER_FLAG);
        else if (i == 0)
            directioner(1);
        else
            empty_instruction(i);
        inject(i);
    }
    jit = (int8_t(*)(void *, void *, void *))(memory_page);
}

void jit_loop(void) {
    initialize_page();
    int8_t not_translated = 1;
    while (not_translated != -1) {
        translate(not_translated - 1);
        inject(not_translated);
        p_code = 0;
        directioner(not_translated);
        inject(0);
        not_translated = jit(template_usage, MEM, regs);
    }
    fprintf(output, "0x%08X->EXIT\n", exitted_normally ? MEM_SIZE : pc + PQP_PADDING + i16);
}

void print_template_usage(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < N_TEMPLATES; i++) {
        fprintf(output, "%02lX:%ld%s", 
                i,
                template_usage[i],
                i == N_TEMPLATES - 1 ? "" : ",");
    }
    fprintf(output, "]\n");
}

void print_regs_state(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < N_REGS; i++) {
        fprintf(output, "R%ld=0x%08X%s",
                i,
                regs[i],
                i == N_REGS - 1 ? "" : "|");
    }
    fprintf(output, "]\n");
}

int main(int argc, char **argv) {
    input = fopen(argv[1], "r");
    length = sysconf(_SC_PAGE_SIZE);
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    for (size_t i = 0; i < MEM_SIZE; i++)
        fscanf(input, "%hhx ", &MEM[i]);
    jit_loop();
    print_template_usage();
    print_regs_state();
    fclose(input);
    fclose(output);
    munmap(memory_page, length);
}
