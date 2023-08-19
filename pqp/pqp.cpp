#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;
using Program = vector<uint8_t>;

#define MEM_SIZE 128
vector<uint8_t> MEM(MEM_SIZE);
#define REGS_SIZE 16
vector<int32_t> regs(REGS_SIZE);
#define INSTRUCTIONS_SIZE 16
vector<uint32_t> instruction_usage(INSTRUCTIONS_SIZE, 0);
#define INSTRUCTION_PADDING 4
vector<bool> instruction_translated(MEM_SIZE / INSTRUCTION_PADDING);
Program code;

// TEMPLATES HELPERS

void emit_byte(uint8_t x) {
    code.push_back(x);
}

void emit_bytes(initializer_list<uint8_t> seq) {
  for (auto v : seq) {
    emit_byte(v);
  }
}

void emit_5_bits(uint32_t x) {
    emit_byte(x & 0b11111);
}

void emit_32_bits(uint32_t x) {
    emit_byte(x & 0xff);
    emit_byte(x >> 8 & 0xff);
    emit_byte(x >> 16 & 0xff);
    emit_byte(x >> 24 & 0xff);
}

void emit_64_bits(uint64_t x) {
    emit_32_bits(x & 0xffffffff);
    emit_32_bits(x >> 32 & 0xffffffff);
}

void emit_nop(uint8_t nop_size) {
    switch (nop_size) {
        case 2:
            // 66 nop
            emit_bytes({0x66, 0x90});
            break;
        case 3:
            // nop DWORD ptr [eax]
            emit_bytes({0x0f, 0x1f, 0x00});
            break;
        case 4:
            // nop DWORD PTR [eax + 0x00]
            emit_bytes({0x0f, 0x1f, 0x40, 0x00});
            break;
        case 5:
            // nop DWORD PTR [eax + eax * 1 + 0x00]
            emit_bytes({0x0f, 0x1f, 0x44, 0x00, 0x00});
            break;
        case 6:
            // 66 nop DWORD PTR [eax + eax* 1 + 0x00]
            emit_bytes({0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00});
            break;
        case 7:
            // nop DWORD PTR [eax + 0x00000000]
            emit_bytes({0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00});
            break;
        case 8:
            // nop DWORD PTR [eax + eax * 1 + 0x00000000]
            emit_bytes({0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00});
            break;
        case 9:
            // 66 nop DWORD PTR [eax + eax * 1 + 0x00000000]
            emit_bytes({0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00});
            break;
        default:
            break;
    }
}

// useful for moving regs addresses to real registers
// emits 20 bytes
void mov_both(void *rx, void *ry) {
    emit_bytes({0x48, 0xb8});   // mov rax, rx
    emit_64_bits((uint64_t)rx);
    emit_bytes({0x48, 0xb9});   // mov rcx, ry
    emit_64_bits((uint64_t)ry);
}

#define ADD 1
#define SUB 2
#define AND 3
#define OR 4
#define XOR 5
/*
 * helper for operations among registers
 * emits 8 bytes
 */
void exec_op(uint8_t op) {
    emit_bytes({0x8b, 0x09});   // mov ecx, DWORD PTR [rcx]
    emit_bytes({0x8b, 0x10});   // mov edx, DWORD PTR [rax]
    switch (op) {
        case ADD:
            emit_bytes({0x01, 0xca}); // add edx, ecx
            break;
        case SUB:
            emit_bytes({0x29, 0xca}); // sub edx, ecx
            break;
        case AND:
            emit_bytes({0x21, 0xca}); // and edx, ecx
            break;
        case OR:
            emit_bytes({0x09, 0xca}); // or edx, ecx
            break;
        case XOR:
            emit_bytes({0x31, 0xca}); // xor edx, ecx
            break;
        default:
            break;
    }
    emit_bytes({0x89, 0x10});   // mov DWORD PTR [rax], edx
}

// INSTRUCTION TEMPLATES

// mov rx, i16
void template_0x0(void *r, uint32_t x) {
    emit_bytes({0x48, 0xb8});   // mov rax, r
    emit_64_bits((uint64_t)r);
    emit_bytes({0xc7, 0x00});   // mov DWORD PTR [rax], x
    emit_32_bits(x);
    for (size_t i = 0; i < 6; i++) emit_nop(8); // pads 48 bytes
}

// mov rx, ry
void template_0x1(void *rx, void *ry) {
    mov_both(rx, ry);
    emit_bytes({0x8b, 0x09});   // mov ecx, DWORD PTR [rcx]
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx
    for (size_t i = 0; i < 5; i++) emit_nop(8); // pads 40 bytes
}

// mov rx, [ry]
void template_0x2(void *rx, void *ry) {
    mov_both(rx, ry);
    emit_bytes({0x48, 0xba});   // mov rdx, MEM
    emit_64_bits((uint64_t)MEM);

    emit_bytes({0x8b, 0x09});   // mov ecx, DWORD PTR [rcx]
    emit_bytes({0x8b, 0x0c, 0x0a}); // mov ecx, DWORD PTR [rdx + rcx]
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx

    for (size_t i = 0; i < 3; i++) emit_nop(8); // pads 27 bytes
    emit_nop(3);
}

// mov [rx], ry
void template_0x3(void *rx, void *ry) {
    mov_both(rx, ry);
    emit_bytes({0x48, 0xba});   // mov rdx, MEM
    emit_64_bits((uint64_t)MEM);

    emit_bytes({0x8b, 0x09});   // mov ecx, DWORD PTR [rcx]
    emit_bytes({0x8b, 0x00});   // mov eax, DWORD PTR [rax]
    emit_bytes({0x89, 0x0c, 0x02}); // mov DWORD PTR [rdx + rax], ecx

    for (size_t i = 0; i < 3; i++) emit_nop(8); // pads 27 bytes
    emit_nop(3);
}

// cmp rx, ry
void template_0x4(void *rx, void *ry) {
    mov_both(rx, ry);
    emit_bytes({0x8b, 0x00});   // mov eax, DWORD PTR [rax]
    emit_bytes({0x8b, 0x09});   // mov ecx, DWORD PTR [rcx]
    emit_bytes({0x39, 0xc8});   // cmp eax, ecx

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 38 bytes
    emit_nop(6);
}

// jmp i16
void template_0x5(int32_t x) {
    emit_byte(0xe9);         // jmp x*16
    emit_32_bits(x * 16);

    for (size_t i = 0; i < 7; i++) emit_nop(8); // pads 59 bytes
    emit_nop(3);
}

// jg i16
void template_0x6(int32_t x) {
    emit_bytes({0x0f, 0x8f}); // jg x*16
    emit_32_bits(x * 16);

    for (size_t i = 0; i < 7; i++) emit_nop(8); // pads 59 bytes
    emit_nop(3);
}

// jl i16
void template_0x7(int32_t x) {
    emit_bytes({0x0f, 0x8c}); // jg x*16
    emit_32_bits(x * 16);

    for (size_t i = 0; i < 7; i++) emit_nop(8); // pads 59 bytes
    emit_nop(3);
}

// je 16
void template_0x8(int32_t x) { // jg x*16
    emit_bytes({0x0f, 0x84});
    emit_32_bits(x * 16);

    for (size_t i = 0; i < 7; i++) emit_nop(8); // pads 59 bytes
    emit_nop(3);
}

// add rx, ry
void template_0x9(void *rx, void *ry) {
    mov_both(rx, ry);
    exec_op(ADD);

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 36 bytes
    emit_nop(4);
}

// sub rx, ry
void template_0xa(void *rx, void *ry) {
    mov_both(rx, ry);
    exec_op(SUB);

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 36 bytes
    emit_nop(4);
}

// and rx, ry
void template_0xb(void *rx, void *ry) {
    mov_both(rx, ry);
    exec_op(AND);

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 36 bytes
    emit_nop(4);
}

// or rx, ry
void template_0xc(void *rx, void *ry) {
    mov_both(rx, ry);
    exec_op(OR);

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 36 bytes
    emit_nop(4);
}

// xor rx, ry
void template_0xd(void *rx, void *ry) {
    mov_both(rx, ry);
    exec_op(XOR);

    for (size_t i = 0; i < 4; i++) emit_nop(8); // pads 36 bytes
    emit_nop(4);
}

// sal rx, i5
void template_0xe(void *rx, int32_t x) {
    emit_bytes({0x48, 0xb8});   // mov rax, rx
    emit_64_bits((uint64_t)rx);
    emit_bytes{(0x8b, 0x08});   // mov ecx, DWORD PTR [rax]
    emit_bytes({0xc1, 0xe1});   // shl ecx, x
    emit_5_bits(x);
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx
}

// sar rx, i5
void template_0xf(void *rx, int32_t x) {
    emit_bytes({0x48, 0xb8});   // mov rax, rx
    emit_64_bits((uint64_t)rx);
    emit_bytes{(0x8b, 0x08});   // mov ecx, DWORD PTR [rax]
    emit_bytes({0xc1, 0xf9});   // sar ecx, x
    emit_5_bits(x);
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx
}

void parse_instruction(
        const Program &program,
        size_t pc,
        int8_t *instruction,
        int32_t *rx,
        int32_t *ry,
        int32_t *i16
    ) {
   *instruction = program[pc];
   *rx = program[pc + 1] & 0xff;
   *ry = program[pc + 1] >> 4 & 0xff;
   *i16 = (int16_t)program[pc + 2];
}

void translate(Program &program) {
    size_t pc = 0;
    int8_t instruction;
    int32_t rx, ry, i16;
    uint8_t cond = 0b000;
    while (pc >= 0 && pc < MEM_SIZE) {
        parse_instruction(program, pc, &instruction, &rx, &ry, &i16);
        instruction_usage[instruction] += 1;
        if (instruction_translated[instruction]) {
            pc += 4;
            continue;
        }
        fprint(output, "%08X->", pc);
        switch (instruction) {
            case 0x0:
                regs[rx] = i16;
                fprintf(output, "MOV R%d=%08X\n", rx, i16);
                template_0x0(&(regs[rx]), i16);
                break;
            case 0x1:
                regs[rx] = regs[ry];
                fprintf(output, "MOV R%d=R%d=%08X\n", rx, ry, regs);
                template_0x1(&(regs[rx]), &(regs[ry]));
                break;
            case 0x2:
                regs[rx] = MEM[regs[ry]];
                fprintf(
                    output, 
                    "MOV R%d=MEM[%02X, %02X, %02X, %02X]=[%02X, %02X, %02X, %02X]\n",
                    rx,
                    regs[ry],
                    regs[ry] + 1,
                    regs[ry] + 2,
                    regs[ry] + 3,
                    MEM[regs[ry]],
                    MEM[regs[ry]] + 1,
                    MEM[regs[ry]] + 2,
                    MEM[regs[ry]] + 3
                );
                template_0x2(&(regs[rx]), &(regs[ry]));
                break;
            case 0x3:
                MEM[regs[rx]] = regs[ry];
                fprintf(
                    output,
                    "MOV MEM[%02X, %02X, %02X, %02X]=R%d=[%02X, %02X, %02X, %02X]\n",
                    regs[rx],
                    regs[rx] + 1,
                    regs[rx] + 2,
                    regs[rx] + 3,
                    ry,
                    MEM[regs[ry]],
                    MEM[regs[ry]] + 1,
                    MEM[regs[ry]] + 2,
                    MEM[regs[ry]] + 3
                );
                template_0x3(&(regs[rx]), &(regs[ry]));
                break;
            case 0x4:
                if (regs[rx] > regs[ry]) {
                    cond = 0b100;
                }
                else if (regs[rx] < regs[ry]) {
                    cond = 0b010;
                }
                else cond = 0b001;
                fprintf(
                    output,
                    "CMP R%d<=>R%d(G=%d,L=%d,E=%d)\n",
                    rx,
                    ry,
                    cond & 0b100,
                    cond & 0b010,
                    cond & 0b001
                );
                template_0x4(&(regs[rx]), &(regs[ry]));
                break;
            case 0x5:
                pc += i16;
                fprintf(
                    output,
                    "JMP %08X\n",
                    pc + INSTRUCTION_PADDING
                );
                template_0x5(i16);
                break;
            case 0x6:
                pc += cond == 0b100 ? i16 : 0;
                fprintf(
                    output,
                    "JG %08X\n",
                    pc + INSTRUCTION_PADDING
                );
                template_0x6(i16);
                break;
            case 0x7:
                pc += cond == 0b010 ? i16 : 0;
                fprintf(
                    output,
                    "JL %08X\n",
                    pc + INSTRUCTION_PADDING
                );
                template_0x7(i16);
                break;
            case 0x8:
                pc += cond == 0b001 ? i16 : 0;
                fprintf(
                    output,
                    "JE %08X\n",
                    pc + INSTRUCTION_PADDING
                );
                template_0x8(i16);
                break;
            case 0x9:
                fprintf(
                    output,
                    "ADD R%d+=R%d=%08X+%08X=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    regs[ry],
                    regs[rx] += regs[ry]
                );
                template_0x9(&(regs[rx]), &(regs[ry]));
                break;
            case 0xa:
                fprintf(
                    output,
                    "SUB R%d-=R%d=%08X-%08X=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    regs[ry],
                    regs[rx] -= regs[ry]
                );
                template_0xa(&(regs[rx]), &(regs[ry]));
                break;
            case 0xb:
                fprintf(
                    output,
                    "AND R%d&=R%d=%08X&%08X=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    regs[ry],
                    regs[rx] &= regs[ry]
                );
                template_0xb(&(regs[rx]), &(regs[ry]));
                break;
            case 0xc:
                fprintf(
                    output,
                    "OR R%d|=R%d=%08X|%08X=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    regs[ry],
                    regs[rx] |= regs[ry]
                );
                template_0xc(&(regs[rx]), &(regs[ry]));
                break;
            case 0xd:
                fprintf(
                    output,
                    "XOR R%d^=R%d=%08X^%08X=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    regs[ry],
                    regs[rx] ^= regs[ry]
                );
                template_0xd(&(regs[rx]), &(regs[ry]));
                break;
            case 0xe:
                fprintf(
                    output,
                    "SAL R%d<<=%d=%08X<<%d=%08X\n",
                    rx,
                    i16,
                    regs[rx],
                    i16,
                    regs[rx] <<= i16
                );
                template_0xe(&(regs[rx]), i16);
                break;
            case 0xf:
                fprintf(
                    output,
                    "SAR R%d>>=%d=%08X>>%d=%08X\n",
                    rx,
                    ry,
                    regs[rx],
                    i16,
                    regs[rx] >>= i16
                );
                template_0xf(&(regs[rx]), i16);
                break;
            default:
                break;
        }
        pc += INSTRUCTION_PADDING;
    }
    fprintf(output, "%08X->EXIT\n", pc);
}

void print_instruction_usage(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < INSTRUCTIONS_SIZE; i++) {
        fprintf(output, "%02X:%d", i, instruction_usage[i]);
    }
    fprint(output, "]\n");
}

void print_regs_state(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < REGS_SIZE; i++) {
        fprintf(output, "R%d:%08X", i, regs[i]);
    }
    fprint(output, "]\n");
}

FILE *input, *output;
int main(int argc, char **argv) {
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    Program program;
    uint8_t byte;
    while (!feof(input)) {
        byte = fgetc(input);
        program.push_back(byte);
    }
    translate(program);
    print_instruction_usage();
    print_regs_state();
    fclose(input);
    fclose(output);
}
