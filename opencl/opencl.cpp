#define CL_TARGET_OPENCL_VERSION 300 
#include <CL/opencl.hpp>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>


float *A, *B, *C;
std::ifstream inputFile;
std::ofstream outputFile;

const std::string kernel = 
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
"       sum = A[i * k + x] * B[x * m + j]);"
"   }"
"   C[i * m + j] = sum;"
"}";

/* OpenCL handling */

cl::Device device;
cl::Context context;
cl::Program program;

/* OpenCL handling */

int main(int argc, char *argv[]) {
    inputFile.open(argv[1]);
    if (!inputFile.is_open()) {
        std::cerr << "Can't open input file " << argv[1] << std::endl;
        exit(EXIT_FAILURE);
    }
    outputFile.open(argv[2]);
}
