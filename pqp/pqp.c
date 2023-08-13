#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// in bytes
#define MEM_SIZE 128
uint8_t memory[MEM_SIZE];

#define REGS_SIZE 16
uint32_t regs[REGS_SIZE];

void read_code(uint8_t *op, uint8_t *r1, uint8_t r2, uint16_t *integer) {
    uint8_t register_byte, i1, i2;
    fscanf(input, "%x %x %x %x", op, &register_byte, &i1, &i2);
    *r1 = register_byte >> 4;
    *r2 = register_byte & 0x0f;
    *integer = ((uint16_t)firstValue << 8) | (uint16_t)secondValue;
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

/*
 * on this context:
 * - G1 refers to the group of registers pre-64-bits
 * - G2 refers to the group of registers post-64-bits
 */

#define MAX_INSTRUCTION_SIZE 6
uint8_t template[MAX_INSTRUCTION_SIZE];
// limit on the template's usable size
// depends on current instruction size in bytes
uint8_t limit;

// frequently needed for treating G1 operands different from G2
uint8_t calc_operand(uint8_t c, uint8_t a, uint8_t r1, uint8_t b, uint8_t r2) {
    return c + a * (r1 < 8 ? r1 : r1 % 8) + b * (r2 < 8 ? r2 : r2 % 8);
}

/*
 * table of output value
 * ----------------------------------
 * r1's G \ r2's G | 1      | 2     |
 *              1  | p1     | p2    |
 *              2  | p3     | p4    |
 */
uint8_t calc_param(uint8_t r1, uint8_t r2, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
    if (r1 < 8 && r2 < 8) return  p1;
    if (r1 < 8 && r2 >= 8) return p2;
    if (r1 >= 8 && r2 < 8) return p3;
    if (r1 >= 8 && r2 >= 8) return p4;
}

// similar to its more general counterpart
uint8_t calc_param_s(uint8_t r, uint8_t p1, uint8_t p2) {
    if (r1 < 8) return p1;
    return p2;
}

void bytes_array(const void *src, uint8_t *dst, size_t dst_len) {
    for (uint8_t i = 0; i < dst_len; i++) {
        dst[i] = (size_t)src >> (8 * i) & 0xff;
    }
}

// mov rx, i16
void template_0x0 (uint8_t rx, int16_t i16) {
    uint8_t val[2], addr[8];
    bytes_array(&(regs[rx]), addr, 8);
    bytes_array((void *)i16, val, 2);
    uint8_t template[] = {
        0x48, 0xc7, 0xc3, val[0], val[1], 0x00, 0x00 // mov rbx, i16
        0x48, 0xb8, addr[0], addr[1], addr[2],       // mov rax, &(regs[rx])
        addr[3], addr[4], addr[5], addr[6], addr[7], 
        0x48, 0x89, 0x18                             // mov QWORD PTR [rax], rbx
    };
}

// mov rx, ry
void template_0x1 (uint8_t r1, uint8_t r2) {
    uint8_t val_r1[4], 
}

// mov rx, [ry]
void template_0x2 (uint8_t r1, uint8_t r2) {
    register uint32_t r2_reg asm(regs[r2]);
    template_0x0(r2, (uint16_t)MEM[r2_reg]);
    exec_template();
    template_0x1(r1, r2);
    /*
    limit = calc_param(r1, r2, 3, 4, 4, 4);
    uint8_t operand = calc_operand(0x00, 8, r1, 1, r2);
    template[0] = 0x67;
    template[1] = calc_param(r1, r2, 0x8b, 0x41, 0x44, 0x45);
    template[2] = calc_param(r1, r2, operand, 0x8b, 0x8b, 0x8b);
    template[3] = operand; // finished 3-sized mov
    if (r2 == 5 || r2 == 13) { // ebp and r13d are special
        limit = 3 + r2 == 13;
        template[limit - 1] += 64; 
        template[limit] = 0x00;
    }
    if (r2 == 4 || r2 == 12) { // so are esp and r12d
        limit = 3 + r2 == 12;
        template[limit] = 24;
    }
    */
}

// mov [rx], ry
void template_0x3 (uint8_t r1, uint8_t r2) {
    register uint32_t r2_reg asm(regs[r2]), r1_reg asm(regs[r1]);
    MEM[r1_reg] = r2_reg;
}

// cmp rx, ry
void template_0x4 (uint8_t r1, uint8_t r2) {
    limit = calc_param(r1, r2, 2, 3, 3, 3);
    uint8_t operand = calc_operand(0xc0, 1, r1, 8, r2);
    template[0] = calc_param(r1, r2, 0x39, 0x44, 0x41, 0x45);
    template[1] = calc_param(r1, r2, operand, 0x39, 0x39, 0x39);
    template[2] = operand;
}

// void template_0x5 (uint8

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
