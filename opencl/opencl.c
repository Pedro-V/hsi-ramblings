#define CL_TARGET_OPENCL_VERSION 300 
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

FILE *input, *output;
float *A, *B, *C;
uint32_t n, k, m;

/* Matrix operations */

void init_matrix(float **matrix, uint32_t *n, uint32_t *m, uint8_t first_init) {
    first_init ? fscanf(input, "%u %u", n, m) : 0;
    // 0-initialization is important for matrix multiplication
    *matrix = calloc(*n * *m, sizeof(**matrix));
}

void free_matrix(float **matrix, uint32_t n) {
    free(*matrix);
}

void read_matrix (float **matrix, uint32_t n, uint32_t m) {
    uint32_t i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            fscanf(input, "%f", *matrix + i + j);
        }
    }
}

void print_matrix(float *matrix, uint32_t n, uint32_t m, size_t index) {
    uint32_t i, j;
    fprintf(output, "M%ld:\n", index);
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            fprintf(output, "%.2f", matrix[i + j]);
        }
        fprintf(output, "\n");
    }
}

/* Matrix operations */

/* OpenCL handling */

cl_device_id pocl_device;
cl_context context;
cl_program program;

char kernel[] = 
"__kernel void multiply_matrix("
"       __global float *A,"
"       __global float *B,"
"       __global float *C,"
"       const unsigned int n,"
"       const unsigned int k,"
"       const unsigned int m"
") {"
"   unsigned int i = get_global_id(0);"
"   unsigned int j = get_global_id(1);"
"   int sum = 0;"
"   for (unsigned int x = 0; x < k; x++) {"
"       sum = 13 //A[i * k + x] * B[x * m + j]);"
"   }"
"   C[i * m + j] = sum;"
"}";

void init_first_device(cl_device_id *device) {
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, device, NULL);
}

void compile_kernel (
    cl_device_id *device,
    const char **kernel_source,
    cl_context *context,
    cl_program *program
) {
    *context = clCreateContext(NULL, 1, device, NULL, NULL, NULL);
    size_t length = 400;//sizeof(*kernel_source) - 1;
    *program = clCreateProgramWithSource(*context, 1, kernel_source, &length, NULL);
    cl_int status = clBuildProgram(*program, 1, device, NULL, NULL, NULL);
    if (status != CL_BUILD_SUCCESS) exit(EXIT_FAILURE);
    return;
}

cl_mem init_buffer(float **M) {
    return clCreateBuffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, 
        n * k * sizeof(**M),
        *M,
        NULL
    );
}

void multiply_matrix(const uint32_t n, const uint32_t k, const uint32_t m) {
    compile_kernel(&pocl_device, (const char **)&kernel, &context, &program);
    cl_mem buffer_A = init_buffer(&A);
    cl_mem buffer_B = init_buffer(&B);
    cl_mem buffer_C = init_buffer(&C);
    cl_kernel kernel = clCreateKernel(program, "multiply_matrix", NULL);
    clSetKernelArg(kernel, 0, n * k * sizeof(*A), buffer_A);
    clSetKernelArg(kernel, 1, k * m * sizeof(*B), buffer_B);
    clSetKernelArg(kernel, 2, n * m * sizeof(*C), buffer_C);
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, pocl_device, NULL, NULL);
    size_t global_work_size[2] = {n, m};
    clEnqueueNDRangeKernel(queue, kernel, 0, global_work_size,
            NULL, NULL, 0, NULL, NULL);
    clEnqueueReadBuffer(queue, buffer_C, CL_TRUE, 0, n * sizeof(*C), C, 
            0, NULL, NULL);
    clFinish(queue);
}

/* OpenCL handling */

int main(int argc, char *argv[]) {
    input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stdout, "Can't open input file\n");
        exit(EXIT_FAILURE);
    }
    output = fopen(argv[2], "w");
    uint32_t num_products;
    fscanf(input, "%u\n", &num_products);
    for (size_t i = 0; i < num_products; i++) {
        init_matrix(&A, &n, &k, 1); read_matrix(&A, n, k);
        init_matrix(&B, &k, &m, 1); read_matrix(&B, k, m);
        init_matrix(&C, &n, &m, 0);
        init_first_device(&pocl_device);
        multiply_matrix(n, k, m);
        print_matrix(C, n, m, i);
    }
    return 0;
}
