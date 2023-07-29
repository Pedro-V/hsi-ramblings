#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

FILE *input, *output;
float **A, **B, **C;

void init_matrix(float ***matrix, uint32_t *n, uint32_t *m, uint8_t first_init) {
    first_init ? fscanf(input, "%u %u\n", n, m) : 0;
    // 0-initialization is important for matrix multiplication
    *matrix = calloc(*n, sizeof(**matrix));
    uint32_t i;
    for (i = 0; i < *n; i++) (*matrix)[i] = calloc(*m, sizeof(***matrix));
}

void free_matrix(float ***matrix, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) free(*matrix)[i];
    free(*matrix);
}

void read_matrix (float ***matrix, uint32_t n, uint32_t m) {
    uint32_t i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            fscanf(input, "%f", (*matrix)[i][j]);
        }
    }
}


void multiply_line (void *p) {

}

void multiply_matrix (uint32_t n, uint32_t m, uint32_t p) {
    
}


int main (int argc, char *argv[]) {
    input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stdout, "Can't open input file\n");
        exit(EXIT_FAILURE);
    }
    output = fopen(argv[2], "w");
    pthread_t *threads;
    uint32_t *parameters;
    uint32_t num_products, n, m, p;
    fscanf(input, "%u\n", &num_products);
    for (i = 0; i < num_products; i++) {
        init_matrix(&A, &n, &m, 1); read_matrix(&A, n, m);
        init_matrix(&B, &m, &p, 1); read_matrix(&B, m, p);
        init_matrix(&C, &n, &p, 0);
        pthread_create(&threads[i], NULL, task, (void *)(&parameters[i]));
        print_matrix(C);
    }
    return 0;
}
