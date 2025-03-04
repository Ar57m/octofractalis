// fract_kernel.cu
#include <cuda_runtime.h>
#include "custom_quaternion.h" 
#include "parser.h"



__device__ inline void setQuaternionValues(const bool juliaset, Quaternion& c, Quaternion& z,
    const double c_real, const double c_imag,
    const double r_part, const double i_part,
    const double z_initial_r, const double z_initial_i,
    const double quaternion_j = 0.0, const double quaternion_k = 0.0) {

    if (juliaset) {
        c = Quaternion(c_real, c_imag);
        z = Quaternion(r_part, i_part, quaternion_j, quaternion_k);
    } else {
        c = Quaternion(r_part, i_part);
        z = Quaternion(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
    }
}

__device__ inline void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const double temp,
    const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y,
    const bool not_escaped, const int top_colors_outside, const int top_colors_lake, const bool lake, const bool lya) {

    const int index = (y * width + x) * 3;
    int it;

    if (not_escaped && lake) {
        it = array_top_colors_lake[static_cast<int>(my_round((temp / (temp + 1.0)) * top_colors_lake))];
    } else if (lya) {
        it = array_top_colors_outside[static_cast<int>((my_round((temp / (temp + top_colors_outside / 10.0)) * top_colors_outside)))];
    } else {
        it = array_top_colors_outside[iteration % (top_colors_outside+1)];
    }

    output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
}


__global__ void fractal_kernel(uint8_t* d_output,
                    const int* d_array_top_colors_outside,
                    const int* d_array_top_colors_lake,
                    const char* d_exp, const size_t exp_size, 
                    const uint16_t width,
                    const uint16_t height,
                    const uint16_t max_iter,
                    const double xmin, const double xmax,
                    const double ymin, const double ymax,
                    const double c_real, const double c_imag,
                    double escape_radius,
                    const bool fast_mode,
                    const bool juliaset,
                    const bool lake,
                    const int top_colors_outside,
                    const int top_colors_lake,
                    const double quaternion_j,
                    const double quaternion_k,
                    const double z_initial_r,
                    const double z_initial_i,
                    double* input_array,
                    const uint32_t array_size)
{

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    const double dx = (xmax - xmin) / width;
    const double dy = (ymax - ymin) / height;
    double point_x = xmin + x * dx;
    double point_y = ymin + y * dy;
    Quaternion z , c, last_it_z;
    uint16_t iteration = 0;
    Quaternion e(2.7182818284590452353602874713526624977572470937000);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion pi(3.141592653589793238462643383279502884197169399375);

    Quaternion it_quat(0.0);
    Quaternion x_quat(static_cast<double>(x), 0.0, 0.0, 0.0);
    Quaternion y_quat(static_cast<double>(y), 0.0, 0.0, 0.0);

    
    ArrayEntry arrEntries[1] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = 1;


    VariableEntry varEntries[8] = {
        {"z", &z},
        {"c", &c},
        {"phi", const_cast<Quaternion*>(&phi)},
        {"pi", const_cast<Quaternion*>(&pi)},
        {"e", const_cast<Quaternion*>(&e)},
        {"It", &it_quat},
        {"y", &y_quat},
        {"x", &x_quat}
    };
    const size_t numVars = 8;
    Parser parser(d_exp, exp_size, varEntries, numVars, arrEntries, numArrays);
    const ASTNode* ast = parser.parse();



    setQuaternionValues(juliaset, c, z, c_real, c_imag, point_x, point_y,
                            z_initial_r, z_initial_i, quaternion_j, quaternion_k);


    double temp = 0;
    bool not_escaped = true;
    

    while ( not_escaped && iteration < max_iter) {
        it_quat = Quaternion(static_cast<double>(iteration));
        
        last_it_z = ast->evaluate();
        if (last_it_z == z && fast_mode) break;
        z = last_it_z;
        temp = z.mag();
        ++iteration;
        not_escaped = (temp < escape_radius);
    }

    update_output(d_output, d_array_top_colors_outside, d_array_top_colors_lake,
                  temp, width, iteration, x, y, not_escaped,
                  top_colors_outside, top_colors_lake, lake, false);
}


extern "C" void fractal_kernel_call(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp, const size_t exp_size,
    const uint16_t width, const uint16_t height, const uint16_t max_iter,
    const double xmin, const double xmax, const double ymin,
    const double ymax, const double c_real, const double c_imag, double escape_radius, const bool fast_mode,
    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
    const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i, 
    double* input_array, const uint32_t array_size) {


    cudaDeviceSetLimit(cudaLimitStackSize, 16384*3);

    uint8_t* d_output = nullptr;
    cudaMalloc((void**)&d_output, width * height * 3 * sizeof(uint8_t));

    double* d_input_array = nullptr;
    cudaMalloc((void**)&d_input_array, array_size * sizeof(double));
    cudaMemcpy(d_input_array, input_array, array_size * sizeof(double), cudaMemcpyHostToDevice);

    // Allocate and copy color arrays to device
    int* d_array_top_colors_outside = nullptr;
    int* d_array_top_colors_lake = nullptr;
    cudaMalloc((void**)&d_array_top_colors_outside, top_colors_outside * sizeof(int));
    cudaMalloc((void**)&d_array_top_colors_lake, top_colors_lake * sizeof(int));
    cudaMemcpy(d_array_top_colors_outside, array_top_colors_outside, top_colors_outside * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_array_top_colors_lake, array_top_colors_lake, top_colors_lake * sizeof(int), cudaMemcpyHostToDevice);

    char* d_exp = nullptr;
    size_t exp_len = exp_size + 1; // +1 for null terminator
    cudaMalloc((void**)&d_exp, exp_len * sizeof(char));
    cudaMemcpy(d_exp, exp, exp_len * sizeof(char), cudaMemcpyHostToDevice);
    // Define grid and block dimensions
    dim3 blockDim(16, 16);
    dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
                (height + blockDim.y - 1) / blockDim.y);

    // Launch the CUDA kernel with device pointers
    fractal_kernel<<<gridDim, blockDim>>>(d_output,
                                        d_array_top_colors_outside,
                                        d_array_top_colors_lake,
                                        d_exp, exp_len,
                                        width, height, max_iter,
                                        xmin, xmax, ymin, ymax,
                                        c_real, c_imag,
                                        escape_radius,
                                        fast_mode,
                                        juliaset,
                                        lake,
                                        top_colors_outside,
                                        top_colors_lake,
                                        quaternion_j,
                                        quaternion_k,
                                        z_initial_r,
                                        z_initial_i,
                                        d_input_array, array_size);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
    }

    // Copy result back and clean up
    cudaDeviceSynchronize();
    cudaMemcpy(output, d_output, width * height * 3 * sizeof(uint8_t), cudaMemcpyDeviceToHost);

    // Free device memory
    cudaFree(d_exp);
    cudaFree(d_output);
    cudaFree(d_array_top_colors_outside);
    cudaFree(d_array_top_colors_lake);
}