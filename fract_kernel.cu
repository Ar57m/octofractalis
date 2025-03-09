// fract_kernel.cu

#include <stdexcept>
#include <cuda_runtime.h>

#include "custom_quaternion.h" 
#include "parser.h"




__device__ inline void setQuaternionValues(const bool juliaset, Quaternion& c, Quaternion& z,
    const DefaultType c_real, const DefaultType c_imag,
    const DefaultType r_part, const DefaultType i_part,
    const DefaultType z_initial_r, const DefaultType z_initial_i,
    const DefaultType quaternion_j = 0.0, const DefaultType quaternion_k = 0.0) {

    if (juliaset) {
        c = Quaternion(c_real, c_imag);
        z = Quaternion(r_part, i_part, quaternion_j, quaternion_k);
    } else {
        c = Quaternion(r_part, i_part);
        z = Quaternion(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
    }
}

__device__ inline void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const DefaultType temp,
    const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y,
    const bool not_escaped, const int top_colors_outside, const int top_colors_lake, const bool lake, const bool lya) {

    const int index = (y * width + x) * 3;
    int it = 0;

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

__device__ inline void update_pendulum_output(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
    const uint16_t x, const uint16_t y, const int attractor_index, const int num_attractors) {

    int index = (y * width + x) * 3;
    int it = array_top_colors_outside[attractor_index % num_attractors];

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
                    const DefaultType xmin, const DefaultType ymin,
                    const DefaultType dx, const DefaultType dy,
                    const DefaultType c_real, const DefaultType c_imag,
                    DefaultType escape_radius,
                    const bool fast_mode,
                    const bool juliaset,
                    const bool lake,
                    const int top_colors_outside,
                    const int top_colors_lake,
                    const DefaultType quaternion_j,
                    const DefaultType quaternion_k,
                    const DefaultType z_initial_r,
                    const DefaultType z_initial_i,
                    double* input_array,
                    const uint32_t array_size)
{

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    DefaultType point_x = xmin + x * dx;
    DefaultType point_y = ymin + y * dy;

    Quaternion pi(3.1415926535897932384626433832795028841971693993751);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion e(2.7182818284590452353602874713526624977572470937000);

    Quaternion z , c, last_it_z;
    uint16_t iteration = 0;


    Quaternion it_quat(0.0);
    Quaternion x_quat(static_cast<DefaultType>(x), 0.0, 0.0, 0.0);
    Quaternion y_quat(static_cast<DefaultType>(y), 0.0, 0.0, 0.0);

    
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


    DefaultType temp = 0;
    bool not_escaped = true;
    

    while ( not_escaped && iteration < max_iter) {
        it_quat = Quaternion(static_cast<DefaultType>(iteration));
        
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





__global__ void lyapunov_kernel(uint8_t* d_output,
                    const int* d_array_top_colors_outside,
                    const int* d_array_top_colors_lake,
                    const char* d_exp, const size_t exp_size, 
                    const uint16_t width,
                    const uint16_t height,
                    const uint16_t max_iter,
                    const DefaultType xmin, const DefaultType ymin,
                    const DefaultType dx, const DefaultType dy,
                    const DefaultType complex_a, const DefaultType complex_b,
                    DefaultType escape_radius,
                    const int top_colors_outside,
                    const int top_colors_lake,
                    const DefaultType quaternion_j,
                    const DefaultType quaternion_k,
                    double* input_array,
                    const uint32_t array_size)
{

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    DefaultType point_x = xmin + x * dx;
    DefaultType point_y = ymin + y * dy;

    Quaternion pi(3.1415926535897932384626433832795028841971693993751);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion e(2.7182818284590452353602874713526624977572470937000);

    Quaternion v, l, temp;
    Quaternion it_quat(0.0);
    Quaternion x_quat(static_cast<DefaultType>(x));
    Quaternion y_quat(0.0);
    Quaternion k_q = 0.0;
    int k = 0;

    ArrayEntry arrEntries[1] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = 1;

    VariableEntry varEntries[9] = {
        {"v", &v},
        {"l", &l},
        {"c", &temp},
        {"phi", const_cast<Quaternion*>(&phi)},
        {"pi", const_cast<Quaternion*>(&pi)},
        {"e", const_cast<Quaternion*>(&e)},
        {"It", &k_q},
        {"y", &y_quat},
        {"x", &x_quat}
    };

    const size_t numVars = 9;
    Parser parser(d_exp, exp_size, varEntries, numVars, arrEntries, numArrays);
    const ASTNode* ast = parser.parse();
    const Quaternion a(0.5 + point_x * 0.5, complex_a, quaternion_j, quaternion_k);
    const Quaternion b(0.5 + point_y * 0.5, complex_b, quaternion_j, quaternion_k);
    l = Quaternion(0.0, 0.0);
    v = Quaternion(0.5, 0.0);

    k = 0;
    y_quat = (static_cast<DefaultType>(y));


    DefaultType l_mag = 0.0;

    while (k < max_iter) {
        it_quat = Quaternion(static_cast<DefaultType>(k));
        if (k % 12 < 6) {
            v = b * v * (1.0 - v);
            temp = b;
            l += (ast->evaluate());
        } else { //     log(mag(c*(1-2*v)))
            v = a * v * (1.0 - v);
            temp = a;
            l += (ast->evaluate());
        }
        l_mag = l.mag();
        if (l_mag > escape_radius) break;
        ++k;
    }
    update_output( d_output, d_array_top_colors_outside, d_array_top_colors_lake, l_mag, width,
    0, x, y, false, top_colors_outside, top_colors_lake, false, true);

}


__global__ void newton_kernel(uint8_t* d_output,
                    const int* d_array_top_colors_outside,
                    const int* d_array_top_colors_lake,
                    const char* d_exp, const size_t exp_size, 
                    const uint16_t width,
                    const uint16_t height,
                    const uint16_t max_iter,
                    const DefaultType xmin, const DefaultType ymin,
                    const DefaultType dx, const DefaultType dy,
                    const DefaultType c_real, const DefaultType c_imag,
                    const bool juliaset,
                    const int top_colors_outside,
                    const int top_colors_lake,
                    const DefaultType quaternion_j,
                    const DefaultType quaternion_k,
                    const DefaultType z_initial_r,
                    const DefaultType z_initial_i,
                    const DefaultType newton_epsilon,
                    double* input_array,
                    const uint32_t array_size)
{

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    DefaultType point_x = xmin + x * dx;
    DefaultType point_y = ymin + y * dy;

    Quaternion pi(3.1415926535897932384626433832795028841971693993751);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion e(2.7182818284590452353602874713526624977572470937000);

    Quaternion z , c, last_it_z;
    uint16_t iteration = 0;


    Quaternion it_quat(0.0);
    Quaternion x_quat(static_cast<DefaultType>(x), 0.0, 0.0, 0.0);
    Quaternion y_quat(static_cast<DefaultType>(y), 0.0, 0.0, 0.0);


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


    DefaultType temp = 0;
    bool not_escaped = true;


    while ( not_escaped && iteration < max_iter) {
        it_quat = Quaternion(static_cast<DefaultType>(iteration));

        const Quaternion last_z = z;
        const DefaultType h(newton_epsilon);
        
        z += h;
        const Quaternion next_z = ast->evaluate();
        z = last_z;
        z = ast->evaluate();
        
        temp = z.mag();
        
        if ( temp < 1e-13 ) break;
        const Quaternion znew = ( next_z - z )/(h); 
        z = last_z - ( z/znew );
        ++iteration;
    }

    update_output( d_output, d_array_top_colors_outside, d_array_top_colors_lake, 3.0, width,
        iteration, x, y, false, top_colors_outside, top_colors_lake, false, false);
}


__global__ void magnet_kernel(uint8_t* d_output,
                    const int* d_array_top_colors_outside,
                    const Quaternion* attractors,
                    const char* d_exp, const size_t exp_size, 
                    const uint16_t width,
                    const uint16_t height,
                    const uint16_t max_iter,
                    const DefaultType xmin, const DefaultType ymin,
                    const DefaultType dx, const DefaultType dy,
                    const DefaultType v_real, const DefaultType v_imag,
                    DefaultType escape_radius,
                    const DefaultType quaternion_j,
                    const DefaultType quaternion_k,
                    const bool fast_mode,
                    const int n_points,
                    double* input_array,
                    const uint32_t array_size)
{

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    DefaultType point_x = xmin + x * dx;
    DefaultType point_y = ymin + y * dy;

    Quaternion pi(3.1415926535897932384626433832795028841971693993751);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion e(2.7182818284590452353602874713526624977572470937000);

    Quaternion z, velocity, force, dif;
    const DefaultType damping = 0.1;
    uint16_t iteration = 0;


    Quaternion it_quat(0.0);
    Quaternion x_quat(static_cast<DefaultType>(x), 0.0, 0.0, 0.0);
    Quaternion y_quat(static_cast<DefaultType>(y), 0.0, 0.0, 0.0);

    
    ArrayEntry arrEntries[1] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = 1;





    VariableEntry varEntries[10] = {
        {"z", &z},
        {"v", &velocity},
        {"f", &force},
        {"dif", &dif},
        {"phi", const_cast<Quaternion*>(&phi)},
        {"pi", const_cast<Quaternion*>(&pi)},
        {"e", const_cast<Quaternion*>(&e)},
        {"It", &it_quat},
        {"y", &y_quat},
        {"x", &x_quat}
    };

    const size_t numVars = 10;
    Parser parser(d_exp, exp_size, varEntries, numVars, arrEntries, numArrays);
    const ASTNode* ast = parser.parse();
    const DefaultType r0 = 0.1;


    Quaternion last_it_z;
    z = Quaternion(point_x, point_y, quaternion_j, quaternion_k);
    velocity = Quaternion(v_real, v_imag);


    DefaultType temp = 0;
    int closest_attractor_index = -1;
    DefaultType min_distance = Max_flt;
    

    while (iteration < max_iter) {
        it_quat = Quaternion(static_cast<DefaultType>(iteration));
        force = Quaternion(0);

        for (int i = 0; i < n_points; ++i) {
            dif = attractors[i] - z;
            DefaultType distance2 = dif.magSquared() + r0 * r0;
            force += dif / distance2;

            if (distance2 < min_distance) {
                min_distance = distance2;
                closest_attractor_index = i;
            }
        }

        force -= velocity * damping;
        velocity += force;
        last_it_z = ast->evaluate();
        if (last_it_z == z && fast_mode) break;
        z = last_it_z;


        temp = z.magSquared();
        if (temp > escape_radius) break;
        ++iteration;
    }
    update_pendulum_output(d_output, d_array_top_colors_outside, width, x, y, closest_attractor_index, n_points);
}


__global__ void generate_lorenz_trajectory(Quaternion* trajectory, const DefaultType sigma, const DefaultType rho, const DefaultType beta, const DefaultType dt,
                        const int max_iter, const char* expression, const size_t exp_size, const DefaultType z_initial_r, const DefaultType z_initial_i,
                        const DefaultType quaternion_j, const DefaultType quaternion_k, double* input_array, const uint32_t array_size) {
    

    Quaternion pi(3.1415926535897932384626433832795028841971693993751);
    Quaternion phi(1.6180339887498948482045868343656381177203091798057);
    Quaternion e(2.7182818284590452353602874713526624977572470937000);

    Quaternion point(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
    Quaternion dx = 0.0;
    Quaternion dy = 0.0;
    Quaternion dz = 0.0;
    Quaternion it_quat = 0.0;
    const Quaternion dt_q = 0.0;

    ArrayEntry arrEntries[1] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = 1;

    VariableEntry varEntries[9] = {
        {"z", &point},
        {"phi", const_cast<Quaternion*>(&phi)},
        {"pi", const_cast<Quaternion*>(&pi)},
        {"e", const_cast<Quaternion*>(&e)},
        {"It", &it_quat},
        {"dx", &dx},
        {"dy", &dy},
        {"dz", &dz},
        {"dt", const_cast<Quaternion*>(&dt_q)}
    };
    
    const size_t numVars = 9;
    Parser parser(expression, exp_size, varEntries, numVars, arrEntries, numArrays);
    const ASTNode* ast = parser.parse(); // dx+dy*1i+dz*1j

    #pragma unroll
    for (int i = 0; i < max_iter; ++i) {
        it_quat = static_cast<DefaultType>(i);
        dx = sigma * (point.imag - point.real) * dt;
        dy = (point.real * (rho - point.j) - point.imag) * dt;
        dz = (point.real * point.imag - beta * point.j) * dt;
        point += ast->evaluate();
        trajectory[i] = point;
    }
}



// Error checking function
inline void checkCudaError(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

// RAII wrapper for CUDA device memory
template <typename T>
class CudaMemory {
public:
    // count is the number of elements of type T.
    CudaMemory(size_t count) : count_(count) {
        checkCudaError(cudaMalloc((void**)&ptr_, count_ * sizeof(T)), "Failed to allocate device memory");
    }
    ~CudaMemory() {
        if (ptr_) {
            cudaFree(ptr_);
        }
    }
    T* get() const { return ptr_; }
    // Disable copy semantics
    CudaMemory(const CudaMemory&) = delete;
    CudaMemory& operator=(const CudaMemory&) = delete;
private:
    T* ptr_ = nullptr;
    size_t count_;
};

// Generalized copy function
template <typename T>
void copyMemory(T* destination, const T* source, size_t count, cudaMemcpyKind direction) {
    const char* errorMsg = (direction == cudaMemcpyHostToDevice) ? "Failed to copy to device"
                               : (direction == cudaMemcpyDeviceToHost) ? "Failed to copy to host"
                               : "Memory copy failed";
    checkCudaError(cudaMemcpy(destination, source, count * sizeof(T), direction), errorMsg);
}





extern "C" void fractal_kernel_call(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp, const size_t exp_size,
    const uint16_t width, const uint16_t height, const uint16_t max_iter,
    const DefaultType xmin, const DefaultType ymin, const DefaultType dx,
    const DefaultType dy, const DefaultType c_real, const DefaultType c_imag, DefaultType escape_radius, const bool fast_mode,
    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
    const DefaultType quaternion_j, const DefaultType quaternion_k, const DefaultType z_initial_r, const DefaultType z_initial_i, 
    double* input_array, const uint32_t array_size) {


        cudaDeviceSetLimit(cudaLimitStackSize, 16384 * 3);

        // Allocate device memory using the RAII wrapper.
        // Note: The count here is the number of elements.
        CudaMemory<uint8_t> d_output(width * height * 3);
        CudaMemory<double> d_input_array(array_size);
        CudaMemory<int> d_array_top_colors_outside(top_colors_outside);
        CudaMemory<int> d_array_top_colors_lake(top_colors_lake);
        
        size_t exp_len = exp_size + 1; // +1 for null terminator
        CudaMemory<char> d_exp(exp_len);
    
        // Copy host data to device
        copyMemory(d_input_array.get(), input_array, array_size, cudaMemcpyHostToDevice);
        copyMemory(d_array_top_colors_outside.get(), array_top_colors_outside, top_colors_outside, cudaMemcpyHostToDevice);
        copyMemory(d_array_top_colors_lake.get(), array_top_colors_lake, top_colors_lake, cudaMemcpyHostToDevice);
        copyMemory(d_exp.get(), exp, exp_len, cudaMemcpyHostToDevice);
    
        // Define grid and block dimensions
        dim3 blockDim(16, 16);
        dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
                     (height + blockDim.y - 1) / blockDim.y);
    
        // Launch the CUDA kernel using the device pointers from the RAII wrappers
        fractal_kernel<<<gridDim, blockDim>>>(d_output.get(),
                                              d_array_top_colors_outside.get(),
                                              d_array_top_colors_lake.get(),
                                              d_exp.get(), exp_len,
                                              width, height, max_iter,
                                              xmin, ymin, dx, dy,
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
                                              d_input_array.get(), array_size);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
        }
    
        // Synchronize the device to ensure kernel completion
        cudaDeviceSynchronize();
    
        // Copy the result from device to host
        copyMemory(output, d_output.get(), width * height * 3, cudaMemcpyDeviceToHost);
    
}




extern "C" void lyapunov_kernel_call(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp, const size_t exp_size,
    const uint16_t width, const uint16_t height, const uint16_t max_iter,
    const DefaultType xmin, const DefaultType ymin, const DefaultType dx,
    const DefaultType dy, DefaultType complex_a, DefaultType complex_b,
    const DefaultType quaternion_j, const DefaultType quaternion_k, DefaultType escape_radius, 
    const int top_colors_outside, const int top_colors_lake, double* input_array, const uint32_t array_size) {

    // Increase the device stack size if needed.
    cudaDeviceSetLimit(cudaLimitStackSize, 16384 * 3);

    // Allocate device memory using our RAII wrapper.
    CudaMemory<uint8_t> d_output(width * height * 3);
    CudaMemory<double> d_input_array(array_size);
    CudaMemory<int> d_array_top_colors_outside(top_colors_outside);
    CudaMemory<int> d_array_top_colors_lake(top_colors_lake);
    size_t exp_len = exp_size + 1; // +1 for null terminator
    CudaMemory<char> d_exp(exp_len);

    // Copy host data to device.
    copyMemory(d_input_array.get(), input_array, array_size, cudaMemcpyHostToDevice);
    copyMemory(d_array_top_colors_outside.get(), array_top_colors_outside, top_colors_outside, cudaMemcpyHostToDevice);
    copyMemory(d_array_top_colors_lake.get(), array_top_colors_lake, top_colors_lake, cudaMemcpyHostToDevice);
    copyMemory(d_exp.get(), exp, exp_len, cudaMemcpyHostToDevice);

    // Define grid and block dimensions.
    dim3 blockDim(16, 16);
    dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
                 (height + blockDim.y - 1) / blockDim.y);

    // Launch the CUDA kernel with device pointers from the RAII wrappers.
    lyapunov_kernel<<<gridDim, blockDim>>>(d_output.get(),
                                           d_array_top_colors_outside.get(),
                                           d_array_top_colors_lake.get(),
                                           d_exp.get(), exp_len,
                                           width, height, max_iter,
                                           xmin, ymin, dx, dy,
                                           complex_a, complex_b,
                                           escape_radius,
                                           top_colors_outside,
                                           top_colors_lake,
                                           quaternion_j,
                                           quaternion_k,
                                           d_input_array.get(), array_size);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
    }

    // Synchronize to ensure kernel completion.
    cudaDeviceSynchronize();

    // Copy the result back from device to host.
    copyMemory(output, d_output.get(), width * height * 3, cudaMemcpyDeviceToHost);


}



extern "C" void newton_kernel_call(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp, const size_t exp_size,
    const uint16_t width, const uint16_t height, const uint16_t max_iter,
    const DefaultType xmin, const DefaultType ymin, const DefaultType dx,
    const DefaultType dy, const DefaultType c_real, const DefaultType c_imag,
    const bool juliaset, const int top_colors_outside, const int top_colors_lake,
    const DefaultType quaternion_j, const DefaultType quaternion_k, const DefaultType z_initial_r, const DefaultType z_initial_i, 
    const DefaultType newton_epsilon, double* input_array, const uint32_t array_size) {


        cudaDeviceSetLimit(cudaLimitStackSize, 16384 * 3);

        // Allocate device memory using the RAII wrapper.
        // Note: The count here is the number of elements.
        CudaMemory<uint8_t> d_output(width * height * 3);
        CudaMemory<double> d_input_array(array_size);
        CudaMemory<int> d_array_top_colors_outside(top_colors_outside);
        CudaMemory<int> d_array_top_colors_lake(top_colors_lake);
        
        size_t exp_len = exp_size + 1; // +1 for null terminator
        CudaMemory<char> d_exp(exp_len);
    
        // Copy host data to device
        copyMemory(d_input_array.get(), input_array, array_size, cudaMemcpyHostToDevice);
        copyMemory(d_array_top_colors_outside.get(), array_top_colors_outside, top_colors_outside, cudaMemcpyHostToDevice);
        copyMemory(d_array_top_colors_lake.get(), array_top_colors_lake, top_colors_lake, cudaMemcpyHostToDevice);
        copyMemory(d_exp.get(), exp, exp_len, cudaMemcpyHostToDevice);
    
        // Define grid and block dimensions
        dim3 blockDim(16, 16);
        dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
                     (height + blockDim.y - 1) / blockDim.y);
    
        // Launch the CUDA kernel using the device pointers from the RAII wrappers
        newton_kernel<<<gridDim, blockDim>>>(d_output.get(),
                                              d_array_top_colors_outside.get(),
                                              d_array_top_colors_lake.get(),
                                              d_exp.get(), exp_len,
                                              width, height, max_iter,
                                              xmin, ymin, dx, dy,
                                              c_real, c_imag,
                                              juliaset,
                                              top_colors_outside,
                                              top_colors_lake,
                                              quaternion_j,
                                              quaternion_k,
                                              z_initial_r,
                                              z_initial_i,
                                              newton_epsilon,
                                              d_input_array.get(), array_size);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
        }
    
        // Synchronize the device to ensure kernel completion
        cudaDeviceSynchronize();
    
        // Copy the result from device to host
        copyMemory(output, d_output.get(), width * height * 3, cudaMemcpyDeviceToHost);
    
}



extern "C" void magnet_kernel_call(uint8_t* output, const int* array_top_colors_outside, const Quaternion* attractors,
    const char* exp, const size_t exp_size, const uint16_t width, const uint16_t height,
    const uint16_t max_iter, const DefaultType xmin, const DefaultType ymin,
    const DefaultType dx, const DefaultType dy, const DefaultType v_real, const DefaultType v_imag,
    DefaultType escape_radius, const DefaultType quaternion_j, const DefaultType quaternion_k, 
    const bool fast_mode, const int n_points, double* input_array, const uint32_t array_size) {


        cudaDeviceSetLimit(cudaLimitStackSize, 16384 * 3);

        // Allocate device memory using the RAII wrapper.
        // Note: The count here is the number of elements.
        CudaMemory<uint8_t> d_output(width * height * 3);
        CudaMemory<double> d_input_array(array_size);
        CudaMemory<int> d_array_top_colors_outside(n_points);
        CudaMemory<Quaternion> d_attractors(n_points);
        
        size_t exp_len = exp_size + 1; // +1 for null terminator
        CudaMemory<char> d_exp(exp_len);
    
        // Copy host data to device
        copyMemory(d_input_array.get(), input_array, array_size, cudaMemcpyHostToDevice);
        copyMemory(d_array_top_colors_outside.get(), array_top_colors_outside, n_points, cudaMemcpyHostToDevice);
        copyMemory(d_exp.get(), exp, exp_len, cudaMemcpyHostToDevice);
        copyMemory(d_attractors.get(), attractors, n_points, cudaMemcpyHostToDevice);
    
        // Define grid and block dimensions
        dim3 blockDim(16, 16);
        dim3 gridDim((width + blockDim.x - 1) / blockDim.x,
                     (height + blockDim.y - 1) / blockDim.y);
    
        // Launch the CUDA kernel using the device pointers from the RAII wrappers
        magnet_kernel<<<gridDim, blockDim>>>(d_output.get(),
                                              d_array_top_colors_outside.get(),
                                              d_attractors.get(),
                                              d_exp.get(), exp_len,
                                              width, height, max_iter,
                                              xmin, ymin, dx, dy,
                                              v_real, v_imag,
                                              escape_radius,
                                              quaternion_j,
                                              quaternion_k,
                                              fast_mode, n_points,
                                              d_input_array.get(), array_size);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
        }
    
        // Synchronize the device to ensure kernel completion
        cudaDeviceSynchronize();
    
        // Copy the result from device to host
        copyMemory(output, d_output.get(), width * height * 3, cudaMemcpyDeviceToHost);
    
}




extern "C" void generate_lorenz_trajectory_kernel(Quaternion* trajectory, const DefaultType sigma, const DefaultType rho, const DefaultType beta, const DefaultType dt,
    const int max_iter, const char* exp, const size_t exp_size, const DefaultType z_initial_r, const DefaultType z_initial_i,
    const DefaultType quaternion_j, const DefaultType quaternion_k, double* input_array, const uint32_t array_size) {


    cudaDeviceSetLimit(cudaLimitStackSize, 16384 * 3);

    // Allocate device memory using the RAII wrapper.
    // Note: The count here is the number of elements.
    CudaMemory<double> d_input_array(array_size);
    CudaMemory<Quaternion> d_trajectory(max_iter);
    
    size_t exp_len = exp_size + 1; // +1 for null terminator
    CudaMemory<char> d_exp(exp_len);

    // Copy host data to device
    copyMemory(d_input_array.get(), input_array, array_size, cudaMemcpyHostToDevice);
    copyMemory(d_exp.get(), exp, exp_len, cudaMemcpyHostToDevice);
    copyMemory(d_trajectory.get(), trajectory, max_iter, cudaMemcpyHostToDevice);
    

    generate_lorenz_trajectory<<<1, 1>>>(d_trajectory.get(),
                    sigma, rho, beta, dt, max_iter,
                    d_exp.get(), exp_size, z_initial_r, z_initial_i,
                    quaternion_j, quaternion_k, d_input_array.get(), array_size);


    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << "\n";
    }

    // Synchronize the device to ensure kernel completion
    cudaDeviceSynchronize();

    // Copy the result from device to host
    copyMemory(trajectory, d_trajectory.get(), max_iter, cudaMemcpyDeviceToHost);

}

