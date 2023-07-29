#define CL_HPP_TARGET_OPENCL_VERSION 300 
#include <CL/opencl.hpp>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>

/* file streams */

std::ifstream input_file;
std::ofstream output_file;

/* matrix handling */

std::vector<float> A, B, C;
uint32_t n, k, m;

void read_matrix(std::vector<float>& M, uint32_t& d1, uint32_t& d2) {
    input_file >> d1 >> d2;
    uint32_t size =  d1 * d2;
    M.reserve(size);
}

void init_matrix(std::vector<float>& M, uint32_t d1, uint32_t d2) {
    float elem;
    uint32_t size = d1 * d2;
    while (size--) {
        input_file >> elem;
        M.push_back(elem);
    }
}

void print_matrix(std::vector<float> M, uint32_t d1, uint32_t d2) {
    for (size_t i = 0; i < d1; i++) {
        for (size_t j = 0; j < d2; j++) {
            output_file << M[i + j];
        }
    }
}

/* matrix handling */

/* OpenCL handling */

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
"       sum += A[i * k + x] * B[x * m + j]);"
"   }"
"   C[i * m + j] = sum;"
"}";


cl::Device device;
cl::Context context;
cl::Program program;

void init_device() {
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    size_t p_index = 0; d_index = 0;
    platforms[p_index].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    device = devices[dindex];
}

void compile_kernel(std::string kernel) {
    cl::Program::Sources source({ kernel });
    context = cl::Context(device);
    program = cl::Program(context, source);
}

void multiply_matrix(float* A, float* B, float* C) {
    compile_kernel(kernel);
    cl::Buffer buffer_A(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, n * k * sizeof(float), A);
    cl::Buffer buffer_B(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, k * m * sizeof(float), B);
    cl::Buffer buffer_C(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, n * m * sizeof(float));
    cl::Kernel kernel(program, "multiply_matrix");
    kernel.setArg(0, buffer_A);
    kernel.setArg(1, buffer_B);
    kernel.setArg(2, buffer_C);
    cl::CommandQueue queue = cl::CommandQueue(context, device);
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(n * m));
    queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, n * m * sizeof(int32_t), C);
    queue.finish();
}

/* OpenCL handling */

int main(int argc, char *argv[]) {
    input_file.open(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Can't open input file " << argv[1] << std::endl;
        exit(EXIT_FAILURE);
    }
    output_file.open(argv[2]);

    uint32_t num_products;
    input_file >> num_matrices;
    std::vector<float> A, B, C;
    for (size_t i = 0; i < num_products; i++) {
        read_matrix(A, n, k); init_matrix(A, n, k);
        read_matrix(B, k, m); init_matrix(B, k, m);
        read_matrix(C, n, m);
        multiply_matrix(A.data(), B.data(), C.data());
        print_matrix(C);
        A.clear(); B.clear(); C.clear();
    }
}
