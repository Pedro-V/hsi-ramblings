#define CL_HPP_TARGET_OPENCL_VERSION 300 
#include <CL/opencl.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <vector>

using namespace std;
/* file streams */

ifstream input_file;
ofstream output_file;

/* matrix handling */

vector<float> A, B, C;
uint32_t n, k, m;

void read_matrix(vector<float>& M, uint32_t& d1, uint32_t& d2) {
    input_file >> d1 >> d2;
    uint32_t size =  d1 * d2;
    M.reserve(size);
}

void init_matrix(vector<float>& M, uint32_t d1, uint32_t d2) {
    float elem;
    uint32_t size = d1 * d2;
    while (size--) {
        input_file >> elem;
        M.push_back(elem);
    }
}

void print_matrix(vector<float> M, uint32_t d1, uint32_t d2, uint32_t index) {
    output_file << "M" << index << ":" << endl;
    for (size_t i = 0; i < d1; i++) {
        for (size_t j = 0; j < d2; j++) {
            output_file << fixed << setprecision(2) << M[i + j] << " ";
        }
        output_file << endl;
    }
}

/* matrix handling */

/* OpenCL handling */

#define MY_LOCAL_SIZE 1
const string kernel = 
"__kernel void multiply_matrix("
"       __global float *A,"
"       __global float *B,"
"       __global float *C,"
"       const unsigned int n,"
"       const unsigned int k,"
"       const unsigned int m"
") {"
"   __local float lA[MY_LOCAL_SIZE][MY_LOCAL_SIZE];"
"   __local float lB[MY_LOCAL_SIZE][MY_LOCAL_SIZE];"
"   unsigned int i = get_global_id(0), li = get_local_id(0);"
"   unsigned int j = get_global_id(1), lj = get_local_id(1);"
"   float sum = 0;"
"   for (unsigned int x = 0; x < k / MY_LOCAL_SIZE; x++) {"
"        lA[li][lj] = A[(i * k) + ((x * MY_LOCAL_SIZE) + lj)];"
"        lB[li][lj] = B[j + ((x * MY_LOCAL_SIZE + lj) * m)];"
"        barrier(CLK_LOCAL_MEM_FENCE);"
"        for (unsigned int y = 0; y < MY_LOCAL_SIZE; y++) {"
"            sum += (lA[li][y] * lB[y][lj];"
"        }"
"        barrier(CLK_LOCAL_MEM_FENCE);"
"   }"
"   C[i * m + j] = j;"
"}";


cl::Device device;
cl::Context context;
cl::Program program;

void init_device() {
    vector<cl::Platform> platforms;
    vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    size_t p_index = 0, d_index = 0;
    platforms[p_index].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    device = devices[d_index];
}

void compile_kernel(string kernel) {
    cl::Program::Sources source({ kernel });
    context = cl::Context(device);
    program = cl::Program(context, source);
    auto build_arg = string("-DMY_LOCAl_SIZE=" + to_string(MY_LOCAL_SIZE)).c_str();
    auto status = program.build(build_arg);
    // Exibindo informações de compilação
    if (status != CL_BUILD_SUCCESS) {
        cout << "Build error log:             " 
            << status << endl 
            << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << endl;
    }
    // Checando se a compilação foi bem sucedida
    assert(status == CL_BUILD_SUCCESS);
}

void multiply_matrix(float* A, float* B, float* C) {
    cl::Buffer buffer_A(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, n * k * sizeof(float), A);
    cl::Buffer buffer_B(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, k * m * sizeof(float), B);
    cl::Buffer buffer_C(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, n * m * sizeof(float));
    cl::Kernel kernel(program, "multiply_matrix");
    kernel.setArg(0, buffer_A);
    kernel.setArg(1, buffer_B);
    kernel.setArg(2, buffer_C);
    kernel.setArg(3, n);
    kernel.setArg(4, k);
    kernel.setArg(5, m);
    auto queue = cl::CommandQueue(context, device);
    queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(n, m),
            cl::NDRange(MY_LOCAL_SIZE, MY_LOCAL_SIZE)
    );
    queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, n * m * sizeof(float), C);
    queue.finish();
}

/* OpenCL handling */

int main(int argc, char *argv[]) {
    input_file.open(argv[1]);
    if (!input_file.is_open()) {
        cerr << "Can't open input file " << argv[1] << endl;
        exit(EXIT_FAILURE);
    }
    output_file.open(argv[2]);
    init_device();
    compile_kernel(kernel);
    uint32_t num_products;
    input_file >> num_products;
    for (size_t i = 0; i < num_products; i++) {
        read_matrix(A, n, k); init_matrix(A, n, k);
        read_matrix(B, k, m); init_matrix(B, k, m);
        C.assign(n * m, 0);
        multiply_matrix(A.data(), B.data(), C.data());
        print_matrix(C, n, m, i);
        A.clear(); B.clear(); C.clear();
    }
}
