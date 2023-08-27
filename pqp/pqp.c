#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <stdarg.h>

#define MEM_SIZE 128
uint8_t MEM[MEM_SIZE];
#define N_REGS 16
int32_t REGS[N_REGS];
#define N_INSTRUCTIONS 16
#define PQP_PADDING 4
#define X86_PADDING 64
uint8_t translated[N_INSTRUCTIONS];
uint32_t usage[N_INSTRUCTIONS];
int8_t *code;
// pointer to last byte we wrote in code
int8_t p_code;
void *memory_page;
#define call_template(hex) template_hex(rx, ry, i16)

#define ADD 1
#define SUB 2
#define AND 3
#define OR 4
#define XOR 5

FILE *input, *output;

void emit_byte(int8_t x) {
    code[p_code] = x;
    p_code++;
}

// C standard asks for at least 1 parameter before elipsis
void emit_bytes(int placeholder, ...) {
    va_list args;
    va_start(args, placeholder);
    emit_byte(va_arg(args, int8_t));
    va_end(args);
}

void emit_5_bits(int32_t x) {
    emit_byte(x & 0b11111);
}

void emit_32_bits(int32_t x) {
    emit_byte(x & 0xff);
    emit_byte(x >> 8 & 0xff);
    emit_byte(x >> 16 & 0xffff);
    emit_byte(x >> 24 & 0xffff);
}

void emit_64_bits(int64_t x) {
    emit_32_bits(x & 0xffffffff);
    emit_32_bits(x >> 32 & 0xffffffff);
}

void emit_prologue(void) {
    emit_byte(0x55);                // push rbp
    emit_bytes(0x48, 0x89, 0xe5);   // mov rbp, rsp
}

void emit_epilogue(void) {
    emit_byte(0x5d);    // pop rbp
    emit_byte(0xc3);    // ret
}

void empty_instruction(int8_t i) {
    p_code = 0;
    emit_prologue();
    emit_byte(0x48, 0xc7, 0xc0, i, 0x00, 0x00, 0x00);   // mov rax, i
    emit_epilogue();
}

void initialize_page (void) {
    uint32_t length = sysconf(_SC_PAGE_SIZE);
    memory = mmap(0, length, PROT_NONE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    for (int8_t i = 0; i < MEM_SIZE / PQP_PADDING; i++) {
        empty_instruction(i);
        inject(i);
    }
}

void translate_continuous_block (int8_t inst) {
    uint8_t code, not_jumped = 1, current_instruction = inst;
    do {
        translate(current_instruction);
        inject(current_instruction);
        not_jumped = is_jump(current_instruction);
        current_instruction++;
    } while (not_jumped);
}

void jit_loop(void) {
    initialize_page();
    int8_t not_translated = 0;
    do {
        translate_continuous_block(not_translated);
        not_translated = jit();
    } while (not_translated != -1);
}

int main(int argc, char **argv) {
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    for (size_t i = 0; i < MEM_SIZE; i++)
        fscanf(input, "%hhx ", &MEM[i]);
    jit_loop();
    print_instruction_usage();
    print_regs_state();
    fclose(input);
    fclose(output);
}
