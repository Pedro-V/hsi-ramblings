#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

const char *command;

FILE *input, *output;
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stdout, "Usage: %s <binary_file> <cleaned_up_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    input = fopen(argv[1], "r");
    output = fopen(argv[2], "w");
    if (!input) {
        exit(EXIT_FAILURE);
    }
    constroi_base();
}
