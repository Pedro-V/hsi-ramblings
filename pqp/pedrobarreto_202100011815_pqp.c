#include <stdio.h>
#include <stdlib.h>

FILE *input, *output;

int main(int argc, char **argv) {
    char MEM[128];
    input = fopen(argv[1], "r");
    if (input == 0) exit(EXIT_FAILURE);
    output = fopen(argv[2], "w");
    size_t i = 0;
    do {
        fscanf(input, "%hhx", &MEM[i]);
        fprintf(output, "0x%02hhx%s",
                MEM[i],
                (i + 1) % 4 == 0 ? "\n" : " ");
        i++;
    } while (!feof(input));
}
