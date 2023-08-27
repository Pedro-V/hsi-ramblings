#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

FILE *input, *output;
using namespace std;
using Program = vector<uint8_t>;

#define MEM_SIZE 128
vector<uint8_t> MEM(MEM_SIZE, 0);
#define REGS_SIZE 16
vector<int32_t> regs(REGS_SIZE, 0);
#define INSTRUCTIONS_SIZE 16
vector<uint32_t> instruction_usage(INSTRUCTIONS_SIZE, 0);
#define INSTRUCTION_PADDING 4
vector<bool> instruction_translated(MEM_SIZE / INSTRUCTION_PADDING, false);
Program code;

// TEMPLATES HELPERS

void emit_byte(uint8_t x, int32_t idx) {
    if (idx == -1)
        code.push_back(x);
    else
        code[idx] = x;
}

void emit_bytes(initializer_list<uint8_t> seq, int32_t idx) {
  for (auto v : seq) {
    emit_byte(v);
  }
}

void emit_5_bits(uint32_t x, int32_t idx) {
    emit_byte(x & 0b11111, idx);
}

# define idx_handler(n) idx == -1 ? -1 : idx + n

void emit_32_bits(uint32_t x, int32_t idx) {
    uint8_t c = idx == -1;
    for (size_t i = 0; i < 4; i++)
        emit_byte(x >> (i * 8) & 0xff, idx_handler(i));
}

void emit_64_bits(uint64_t x, int32_t idx) {
    emit_32_bits(x & 0xffffffff, idx_handler(0));
    emit_32_bits(x >> 32 & 0xffffffff, idx_handler(4));
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
    emit_64_bits((uint64_t)MEM.data());

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
    emit_64_bits((uint64_t)MEM.data());

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
    emit_bytes({0x8b, 0x08});   // mov ecx, DWORD PTR [rax]
    emit_bytes({0xc1, 0xe1});   // shl ecx, x
    emit_5_bits(x);
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx
}

// sar rx, i5
void template_0xf(void *rx, int32_t x) {
    emit_bytes({0x48, 0xb8});   // mov rax, rx
    emit_64_bits((uint64_t)rx);
    emit_bytes({0x8b, 0x08});   // mov ecx, DWORD PTR [rax]
    emit_bytes({0xc1, 0xf9});   // sar ecx, x
    emit_5_bits(x);
    emit_bytes({0x89, 0x08});   // mov DWORD PTR [rax], ecx
}

void parse_instruction(
        uint32_t pc,
        int8_t *instruction,
        int32_t *rx,
        int32_t *ry,
        int32_t *i16
    ) {
   *instruction = MEM[pc];
   *rx = MEM[pc + 1] >> 4;
   *ry = MEM[pc + 1] & 0x0f;
   *i16 = (int32_t)*(int16_t *)(&MEM[pc + 2]);
}

void code_startup(void) {
    for (size_t i = 0; i < MEM_SIZE / INSTRUCTION_PADDING; i++) {
        emit_bytes({0x48, 0xc7});   // mov rax, i
        emit_32_bits((int32_t)i);
    }
}

void translate(void) {
    uint32_t pc = 0;
    int8_t instruction;
    int32_t rx, ry, i16;
    uint8_t cond = 0b000;
    while (pc >= 0 && pc < MEM_SIZE) {
        parse_instruction(pc, &instruction, &rx, &ry, &i16);
        instruction_usage[instruction] += 1;
        if (instruction_translated[pc / 4]) {
            pc += 4;
            continue;
        }
        instruction_translated[pc / 4] = true;
        fprintf(output, "0x%08X->", pc);
        switch (instruction) {
            case 0x0:
                regs[rx] = i16;
                fprintf(output, "MOV R%d=0x%08X\n", rx, i16);
                template_0x0(&(regs[rx]), i16);
                break;
            case 0x1:
                regs[rx] = regs[ry];
                fprintf(output, "MOV R%d=R%d=0x%08X\n", rx, ry, regs[ry]);
                template_0x1(&(regs[rx]), &(regs[ry]));
                break;
            case 0x2:
                regs[rx] = *(int32_t *)(&MEM[regs[ry]]);
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
                template_0x2(&(regs[rx]), &(regs[ry]));
                break;
            case 0x3:
                MEM[regs[rx]] = regs[ry];
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
                    (cond & 0b100) >> 2,
                    (cond & 0b010) >> 1,
                    (cond & 0b001)
                );
                template_0x4(&(regs[rx]), &(regs[ry]));
                break;
            case 0x5:
                pc += i16;
                fprintf(
                    output,
                    "JMP 0x%08X\n",
                    pc + INSTRUCTION_PADDING
                );
                template_0x5(i16);
                break;
            case 0x6:
                fprintf(
                    output,
                    "JG 0x%08X\n",
                    pc + INSTRUCTION_PADDING + i16
                );
                pc += cond == 0b100 ? i16 : 0;
                template_0x6(i16);
                break;
            case 0x7:
                fprintf(
                    output,
                    "JL 0x%08X\n",
                    pc + INSTRUCTION_PADDING + i16
                );
                pc += cond == 0b010 ? i16 : 0;
                template_0x7(i16);
                break;
            case 0x8:
                fprintf(
                    output,
                    "JE 0x%08X\n",
                    pc + INSTRUCTION_PADDING + i16
                );
                pc += cond == 0b001 ? i16 : 0;
                template_0x8(i16);
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
                regs[rx] += regs[ry];
                template_0x9(&(regs[rx]), &(regs[ry]));
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
                regs[rx] -= regs[ry];
                template_0xa(&(regs[rx]), &(regs[ry]));
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
                regs[rx] &= regs[ry];
                template_0xb(&(regs[rx]), &(regs[ry]));
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
                regs[rx] |= regs[ry];
                template_0xc(&(regs[rx]), &(regs[ry]));
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
                regs[rx] ^= regs[ry];
                template_0xd(&(regs[rx]), &(regs[ry]));
                break;
            case 0xe:
                i16 >>= 8;
                fprintf(
                    output,
                    "SAL R%d<<=%d=0x%08X<<%d=0x%08X\n",
                    rx,
                    i16,
                    regs[rx],
                    i16,
                    regs[rx] << i16
                );
                regs[rx] <<= i16;
                template_0xe(&(regs[rx]), i16);
                break;
            case 0xf:
                i16 >>= 8;
                fprintf(
                    output,
                    "SAR R%d>>=%d=0x%08X>>%d=0x%08X\n",
                    rx,
                    i16,
                    regs[rx],
                    i16,
                    regs[rx] >> i16
                );
                regs[rx] >>= i16;
                template_0xf(&(regs[rx]), i16);
                break;
            default:
                break;
        }
        pc += INSTRUCTION_PADDING;
    }
    fprintf(output, "0x%08X->EXIT\n", pc);
}

void print_instruction_usage(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < INSTRUCTIONS_SIZE; i++) {
        fprintf(output, "%02lX:%d%s", 
                i,
                instruction_usage[i],
                i == INSTRUCTIONS_SIZE - 1 ? "" : ",");
    }
    fprintf(output, "]\n");
}

void print_regs_state(void) {
    fprintf(output, "[");
    for (size_t i = 0; i < REGS_SIZE; i++) {
        fprintf(output, "R%ld=0x%08X%s",
                i,
                regs[i],
                i == REGS_SIZE - 1 ? "" : "|");
    }
    fprintf(output, "]\n");
}

int main(int argc, char **argv) {
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    for (size_t i = 0; i < MEM_SIZE; i++) {
        fscanf(input, "%hhx ", &MEM[i]);
    }
    translate();
    print_instruction_usage();
    print_regs_state();
    fclose(input);
    fclose(output);
}
