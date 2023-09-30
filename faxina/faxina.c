#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#define STREAM stdin

FILE *input, *output;
int32_t length;
// isso vai ser local pra cada thread
int8_t (*jit)(void *);

typedef struct Instruction {
    uint32_t start_index;
    uint8_t* bytes;
    uint8_t num_bytes;
    uint8_t is_useless;
} Instruction;

typedef struct Code {
    instruction* instructions;
    uint32_t num_instructions;
    uint32_t num_bytes;
} Code;

void get_raw_code(Code *code, uint8_t *result_p) {
    size_t idx = 0;
    instruction curr_instruction;
    for (size_t i = 0; i < code->num_instructions) {
        curr_instruction = code->instructions[i];
        for (size_t j = 0; j < curr_instruction->num_bytes; i++) {
            result_p[idx] = curr_instruction->bytes[j];
            idx++;
        }
    }
}

Code* input_code = malloc(sizeof(Code));

uint32_t read_instruction(Instruction* instruction, uint32_t start_index) {
    fscanf(input, "%d", instruction->num_bytes);
    instruction->bytes = malloc(sizeof(uint8_t) * instruction->num_bytes);
    for (size_t i = 0; i < instruction->num_bytes; i++) {
        fscanf(input, "%hhx", instruction->bytes[i]);
    }
    return start_index + instruction->num_bytes;
}

void read_code(void) {
    fscanf(input, "%d", input_code->num_instructions);
    input_code->instructions = malloc(sizeof(Instruction) * input_code->num_instructions);
    uint32_t idx = 0;
    for (size_t i = 0; i < input_code->num_instructions; i++) {
        idx = read_instruction(input_code->instructions[i], idx);
    }
    input_code->num_bytes = idx;
}

void *verify_instruction(void *i) {
}

make_faxineiros(void) {
    pthread_t *threads;
    for (size_t i = 0; i < input_code->num_instructions; i++) {
        pthread_create(&threads[i], NULL, verify_instruction, i);
    }
}

void faxinar(void) {
    make_faxineiros();
    write_output();
}

int main(int argc, char **argv) {
    uint32_t num_instructions;
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    read_code();
    faxinar();
}
