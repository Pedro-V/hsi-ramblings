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

uint32_t read_instruction(Instruction* instruction, uint32_t start_index) {
    fscanf(input, "%d", instruction->num_bytes);
    instruction->bytes = malloc(sizeof(uint8_t) * instruction->num_bytes);
    for (size_t i = 0; i < instruction->num_bytes; i++) {
        fscanf(input, "%hhx", instruction->bytes[i]);
    }
    return start_index + instruction->num_bytes;
}

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

Code* input_code;

void read_code(void) {
    fscanf(input, "%d", input_code->num_instructions);
    input_code->instructions = malloc(sizeof(Instruction) * input_code->num_instructions);
    uint32_t idx = 0;
    for (size_t i = 0; i < input_code->num_instructions; i++) {
        idx = read_instruction(input_code->instructions[i], idx);
    }
    input_code->num_bytes = idx;
}

uint8_t is_syscall(uint8_t *bytes) {
    return bytes[0] == 0x0f && bytes[1] == 0x05;
}

// copies and substitutes syscalls
void copy_code(Code *temp_code) {
    temp_code->num_instructions = input_code->num_instructions;
    temp_code->num_bytes = input_code->num_bytes;
    temp_code->instructions = malloc(sizeof(Instruction) * input_code->num_instructions);
    for (size_t i = 0; i < input_code->num_instructions; i++) {
       if (is_syscall(input_code->instructions[i].bytes)) {
           temp_code->instructions[i].bytes h
       }
       else {
           memcpy(&temp_code->instructions[i],
                  &input_code->instructions[i],
                  sizeof(input_code->instructions[i]));
       }
    }
}

void *verify_instruction(void *i) {
    Code *temp_code = malloc(sizeof(Code));
    copy_code(temp_code);
    jit();
    int different = compare_outputs();
    if (different) {
        input_code->instructions[i]->is_useless = 1;
    }
    return NULL;
}

void make_faxineiros(void) {
    pthread_t *threads;
    for (size_t i = 0; i < input_code->num_instructions; i++) {
        pthread_create(&threads[i], NULL, verify_instruction, i);
    }
}

void print_instruction(Instruction instruction) {
    for (size_t i = 0; i < instruction.num_bytes; i++) {
        fprintf(output, "%hhx ", instruction.bytes[i]);
    }
    fprintf(output, "\n");
}

void write_useful(void) {
    for (size_t i = 0; i < input_code->num_instructions; i++) {
        if (input_code->instructions[i]->is_useless) continue;
        print_instruction(input_code->instructions[i]);
    }
}

void faxinar(void) {
    make_faxineiros();
    write_useful();
}

int main(int argc, char **argv) {
    uint32_t num_instructions;
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    input_code = malloc(sizeof(Code));
    read_code();
    faxinar();
}
