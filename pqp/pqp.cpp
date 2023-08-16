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
vector<uint32_t> regs(REGS_SIZE);
vector<uint8_t> code;

void emit_byte(uint8_t x) {
    code.push_back(x);
}

void emit_bytes(initializer_list<uint8_t> seq) {
  for (auto v : seq) {
    emit_byte(v);
  }
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

void mov_to_reg(void *r, uint32_t x) {
    emit_bytes({0x48, 0xb8}); // mov rax, r
    emit_64_bits((uint64_t)r);
    emit_bytes({0xc7, 0x00}); // mov DWORD PTR [rax], x
    emit_32_bits(x);
}

void cmp(void *r, int32_t x) {
    emit_bytes({0x48, 0xb8}); // mov rax, r
    emit_64_bits((uint64_t)r);
    emit_bytes({0x48, 0x3d}); // cmp rax, x
    emit_32_bits(x);
}

void jmp(int32_t x) {
    emit_byte(0xe9);         // jmp x*16
    emit_32_bits(x * 16);
    emit_bytes({             // nop 9 bytes
        0x66, 0x0f, 0x1f, 0x84,
        0x00, 0x00, 0x00, 0x00,
        0x00
    });
    emit_bytes({0x66, 0x90}); // nop 2 bytes
}

void parse_instruction(
        const Program &program,
        size_t pc,
        int8_t *instruction,
        int32_t *r1,
        int32_t *r2,
        int32_t *i16
    ) {
   *instruction = program[pc];
   *r1 = program[pc + 1] & 0xff;
   *r2 = program[pc + 1] >> 4 & 0xff;
   *i16 = (int16_t)program[pc + 2];
}

void translate(Program &program) {
    size_t pc = 0;
    int8_t instruction;
    int32_t r1, r2, i16;
    while (pc >= 0 && pc < MEM_SIZE) {
        parse_instruction(program, pc, &instruction, &r1, &r2, &i16);
        switch (instruction) {
            case 0x0:
                mov_to_reg(&r1, i16);
                break;
            case 0x1:
                mov_to_reg(&r1, regs[r2]);
                break;
            case 0x2:
                mov_to_reg(&r1, MEM[regs[r2]]);
                break;
            case 0x3:
                mov_to_reg(&(MEM[regs[r1]]), regs[r2]);
                break;
            case 0x4:
                cmp(&r1, regs[r2]);
                break;
            case 0x5:
                jmp(i16);
                pc += i16;
                break;
            case 0x6:
                break;
            case 0x7:
                break;
            case 0x8:
                break;
            case 0x9:
                mov_to_reg(&r1, regs[r1] + regs[r2]);
                break;
            case 0xa:
                mov_to_reg(&r1, regs[r1] - regs[r2]);
                break;
            case 0xb:
                mov_to_reg(&r1, regs[r1] & regs[r2]);
                break;
            case 0xc:
                mov_to_reg(&r1, regs[r1] | regs[r2]);
                break;
            case 0xd:
                mov_to_reg(&r1, regs[r1] ^ regs[r2]);
                break;
            case 0xe:
                mov_to_reg(&r1, regs[r1] << i16);
                break;
            case 0xf:
                mov_to_reg(&r1, regs[r1] >> i16);
                break;
            default:
                break;
        }
        pc += 4;
    }
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
}
