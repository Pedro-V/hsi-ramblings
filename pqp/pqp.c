#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// in bytes
#define MEM_SIZE 128
uint8_t memory[MEM_SIZE];

#define REGS_SIZE 16
uint32_t regs[REGS_SIZE];

#define CFLAGS_SIZE 3 // cflags = [G, L, E, -]
uint8_t cflags[CFLAGS_SIZE], pc;

void read_code(uint8_t *op, uint8_t *r1, uint8_t r2, uint16_t *integer) {
    uint8_t register_byte, i1, i2;
    fscanf(input, "%x %x %x %x", op, &register_byte, &i1, &i2);
    *r1 = register_byte >> 4;
    *r2 = register_byte & 0x0f;
    *integer = ((uint16_t)firstValue << 8) | (uint16_t)secondValue;
}

void set_cflags(uint8_t r1, uint8_t r2) {
    uint8_t x = regs[r1] - regs[r2];
    if (x > 0) cflags[0] = 1;
    else if (x == 0) cflags[1] = 1;
    else cflags[2] = 1;
}

uint8_t getUpper4Bits(uint8_t value) {
    return (value >> 4) & 0x0F;
}

uint8_t getLower4Bits(uint8_t value) {
    return value & 0x0F;
}

uint8_t getUpperByte(uint16_t value) {
    return (value >> 8) & 0xFF;
}

uint8_t getLowerByte(uint16_t value) {
    return value & 0xFF;
}


/* Templates */ 

void update_template(uint8_t new_instruction[], uint8_t new_limit) {
    limit = new_limit;
    for (size_t i = 0; i < limit; i++) {
        instruction[i] = new_instruction[i];
    }
}

#define MAX_INSTRUCTION_SIZE 6
uint8_t instruction[MAX_INSTRUCTION_SIZE];
uint8_t limit;

void template_0x0 (uint8_t r_index, uint16_t val) {
    uint8_t new_limit, template[n];
    if (r_index < 8) {
        new_limit = 5, new_instruction[limit] = {

        };
    }
    else if (r_index < 16) 
}

void interpret(void) {
    uint8_t op, r1, r2;
    uint16_t integer;
    while (feof(input)) {
        read_code(&op, &r1, &r2, &integer);
        switch (op) {
            case 0x0: // mov r1, integer
                regs[r1] = integer;
                break;
            case 0x1: // mov r1, r2
                regs[r1] = regs[r2];
                break;
            case 0x2: // mov r1, [r2]
                regs[r1] = memory[r2];
                break;
            case 0x3: // mov [r1], r2
                memory[r1] = r2;
                break;
            case 0x4: // cmp r1, r2
                set_cflags();
                break;
            case 0x5: // jmp integer
                pc 
                break;
            case 0x6: // jg integer
                regs;
                break;
            case 0x7: // jl integer
                regs;
                break;
            case 0x8: // je integer
                regs;
                break;
            case 0x9: // add r1, r2
                regs;
                break;
            case 0xa: // sub r1, r2
                regs;
                break;
            case 0xb: // and r1, r2
                regs;
                break;
            case 0xc: // or r1, r2
                regs;
                break;
            case 0xd: // xor r1, r2
                regs;
                break;
            case 0xe: // shl r1, integer
                regs;
                break;
            case 0xf  // shr r1, integer
                regs;
                break;
            case default:
                fprint(stdout, "instruction not supported");
                break;
        }
    }
}

void print_instruction_count(void) {};
void print_register_state(void) {};


void print_final_state(void) {
    print_instruction_count();
    print_register_state();
}

FILE input, output;

int main(int argc, char **argv) {
    input = fopen(argv[1], "r");
    if (!input) {
        fprintf("Can't read input file");
        exit(EXIT_FAILURE);
    }
    output = fopen(argv[2], "w");
    interpret();
    print_final_state();
}
