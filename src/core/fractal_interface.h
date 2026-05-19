#ifndef FRACTAL_INTERFACE_H
#define FRACTAL_INTERFACE_H

#include <cstdint>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <iomanip>
#include <functional>
#include <stdexcept>
#include <atomic>

#ifndef USE_CUDA
#include <omp.h>
#include "core/parser.h"
#endif







// class ManualTimer {
// public:
//     ManualTimer(std::string logPath) : m_logPath(std::move(logPath)) {}

//     // Start or restart the timer for a specific phase
//     void start(std::string taskName) {
//         m_taskName = std::move(taskName);
//         m_startTime = std::chrono::high_resolution_clock::now();
//     }

//     // Stop and write to the log immediately
//     void stop() {
//         auto endTime = std::chrono::high_resolution_clock::now();
        
//         auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - m_startTime).count();
//         double ms = duration / 1000000.0; // High precision conversion to milliseconds

//         std::ofstream logFile(m_logPath, std::ios_base::app);
//         if (logFile.is_open()) {
//             logFile << "[" << m_taskName << "] " 
//                     << std::fixed << std::setprecision(4) << ms << " ms" << std::endl;
//         }
//     }

// private:
//     std::string m_logPath;
//     std::string m_taskName;
//     std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
// };





// Portable nearest-even rounding
inline int round_div_nearest_even(long long numer, int denom) {
    long long q = numer / denom;
    long long rem = numer % denom;
    long long twice = rem * 2;
    if (twice < denom) return (int)q;
    if (twice > denom) return (int)(q + 1);
    return (int)((q % 2 == 0) ? q : q + 1);
}

// CPU version of the RGB Lerp
inline uint32_t rgbLerp_cpu(uint32_t c0, uint32_t c1, int step, int steps) {
    int r0 = (c0 >> 16) & 0xFF;
    int g0 = (c0 >> 8) & 0xFF;
    int b0 = c0 & 0xFF;
    int r1 = (c1 >> 16) & 0xFF;
    int g1 = (c1 >> 8) & 0xFF;
    int b1 = c1 & 0xFF;
    
    int denom = steps + 1;
    int r = round_div_nearest_even((long long)r0 * (denom - step) + (long long)r1 * step, denom);
    int g = round_div_nearest_even((long long)g0 * (denom - step) + (long long)g1 * step, denom);
    int b = round_div_nearest_even((long long)b0 * (denom - step) + (long long)b1 * step, denom);
    
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

void generate_on_cpu(const uint32_t* h_colors, int numColors, int totalOutputSize, uint32_t* h_out) {
    if (totalOutputSize <= 0 || numColors <= 0) return;

    int outSizePerColor = totalOutputSize / numColors;
    int S = outSizePerColor - 1;

    for (int i = 0; i < totalOutputSize; ++i) {
        int colorIndex = i / outSizePerColor;
        int pos = i % outSizePerColor;

        if (colorIndex >= numColors) break;

        uint32_t c0 = h_colors[colorIndex];
        if (pos == 0 || S <= 0) {
            h_out[i] = c0;
        } else {
            uint32_t c1 = h_colors[(colorIndex + 1) % numColors];
            h_out[i] = rgbLerp_cpu(c0, c1, pos, S);
        }
    }
}



#ifndef USE_CUDA

static inline void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const DefaultType& temp,
                const uint16_t& width, const uint16_t& iteration, const uint16_t& x, const uint16_t& y,
                const bool& not_escaped, const int& top_colors_outside, const int& top_colors_lake, const bool& lake, const bool& lya) {

    const int index = (y * width + x) * 4;
    int it = 0;
    
    if (not_escaped) {
        it = lake ? array_top_colors_lake[static_cast<int>(my_round((temp / (temp + 1.0)) * (top_colors_lake - 1)))] : array_top_colors_lake[0];
    } else if (lya) {
        it = array_top_colors_outside[static_cast<int>((my_round((temp / (temp + top_colors_outside / 10.0)) * (top_colors_outside - 1))))];
    } else {
        it = array_top_colors_outside[iteration % top_colors_outside];
    }

    output[index]     = static_cast<uint8_t>((it >> 16) & 0xFF); // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);  // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);         // B
    output[index + 3] = 255;                                     // A
}


void update_pendulum_output(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
                            const uint16_t x, const uint16_t y, const int attractor_index, const int num_attractors) {

    int index = (y * width + x) * 3;
    int it = array_top_colors_outside[attractor_index % num_attractors];

    output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
}




template <int Dim>
void setHypercomplexValues(
    bool juliaset,
    Hypercomplex<Dim>& c,
    Hypercomplex<Dim>& z,
    const double* juliaset_c,
    DefaultType real_part,
    DefaultType imag_part,
    const double* z_initial)
{
    // Helper to copy from double array to Hypercomplex<Dim> components
    auto setFromDoubleArray = [](Hypercomplex<Dim>& h, const double* src) {
        for (int i = 0; i < Dim; ++i)  // max 8 components
            h.v[i] = static_cast<DefaultType>(src[i]);
    };

    if (juliaset) {
        // c is constant from juliaset_c, z uses (real_part, imag_part) + extra from z_initial
        setFromDoubleArray(c, juliaset_c);
        z.v[0] = real_part;
        z.v[1] = imag_part;
        for (int i = 2; i < Dim; ++i)
            z.v[i] = static_cast<DefaultType>(z_initial[i]);
    } else {
        // c uses (real_part, imag_part) + extra from juliaset_c[2..]
        c.v[0] = real_part;
        c.v[1] = imag_part;
        for (int i = 2; i < Dim; ++i)
            c.v[i] = static_cast<DefaultType>(juliaset_c[i]);

        // z is fully initialised from z_initial
        setFromDoubleArray(z, z_initial);
    }
}


template <int Dim>
void runFractalCPU(
    uint8_t* output,
    const int* array_top_colors_outside,
    const int* array_top_colors_lake,
    const char* exp,
    size_t exp_size,
    uint16_t width, uint16_t height,
    uint16_t max_iter,
    double xmin, double ymin, double dx, double dy,
    const double* juliaset_c,
    double escape_radius,
    bool fast_mode, bool juliaset, bool lake, bool ignore_it, std::atomic<bool>& stopFlag,
    int top_colors_outside, int top_colors_lake,
    const double* z_initial,
    double* input_array,
    uint32_t array_size)
{
    using H = Hypercomplex<Dim>;

    // Constants
    H pi(3.14159265358979323846);
    H phi(1.61803398874989484820);
    H e(2.71828182845904523536);

    // Pre‑parse the expression (outside parallel region)
    // Variables that are updated per pixel: z, c, it, x, y
    // We'll point to dummy values during parse (nullptr), then rebind per thread.
    VariableEntry<Dim> baseVarEntries[] = {
        {"z",   nullptr},
        {"c",   nullptr},
        {"phi", const_cast<H*>(&phi)},
        {"pi",  const_cast<H*>(&pi)},
        {"e",   const_cast<H*>(&e)},
        {"it",  nullptr},
        {"y",   nullptr},
        {"x",   nullptr}
    };
    const size_t numVars = sizeof(baseVarEntries) / sizeof(baseVarEntries[0]);

    ArrayEntry arrEntries[] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = sizeof(arrEntries) / sizeof(arrEntries[0]);

    Parser<Dim> parser(exp, exp_size, baseVarEntries, numVars, arrEntries, numArrays);
    parser.parse();
    const Instruction<Dim>* globalBytecode = parser.getBytecode();
    const size_t bytecodeSize = parser.getBytecodeSize();

    if (bytecodeSize == 0) {
        printf("Error: empty bytecode from expression: %s\n", exp);
        fflush(stdout);
        return;
    }

    // Pixel loop with OpenMP
    #pragma omp parallel for schedule(dynamic)
    for (int x = 0; x < width; ++x) {
        if (stopFlag.load(std::memory_order_relaxed)) continue;

        H z, c, last_it_z;
        H it_quat(0.0);
        H x_quat(static_cast<DefaultType>(x));
        H y_quat(0.0);

        VariableEntry<Dim> threadVarEntries[] = {
            {"z",   &z},
            {"c",   &c},
            {"phi", const_cast<H*>(&phi)},
            {"pi",  const_cast<H*>(&pi)},
            {"e",   const_cast<H*>(&e)},
            {"it",  &it_quat},
            {"y",   &y_quat},
            {"x",   &x_quat}
        };

        for (int y = 0; y < height; ++y) {
            if (stopFlag.load(std::memory_order_relaxed)) break;

            y_quat = H(static_cast<DefaultType>(y));

            setHypercomplexValues<Dim>(juliaset, c, z, juliaset_c,
                                    xmin + x * dx,
                                    ymin + y * dy,
                                    z_initial);

            uint16_t iteration = 0;
            DefaultType magSq = 0.0;
            bool escaped = false;

            while (!escaped && iteration < max_iter) {
                if ((iteration & 7) == 0) { // check every 8 iterations
                    if (stopFlag.load(std::memory_order_relaxed)) break;
                }

                it_quat = H(static_cast<DefaultType>(iteration));

                evaluateBytecode(last_it_z, globalBytecode, bytecodeSize,
                                threadVarEntries, arrEntries);

                if (fast_mode && last_it_z == z) break;

                z = last_it_z;
                H::mag_squared(magSq, z);
                ++iteration;
                escaped = (magSq >= escape_radius);
            }

            if (stopFlag.load(std::memory_order_relaxed)) break;

            update_output(output, array_top_colors_outside, array_top_colors_lake,
                        my_sqrt(magSq), width,
                        iteration, x, y, ignore_it ? true : !escaped,
                        top_colors_outside, top_colors_lake, lake, false);
        }
    }
}
#endif


#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif














extern "C" {
    void fractal_kernel_call(uint8_t* output, const int* array_top_colors_outside,
        const int* array_top_colors_lake, const char* exp, const size_t exp_size,
        const uint16_t width, const uint16_t height, const uint16_t max_iter,
        const DefaultType xmin, const DefaultType ymin, const DefaultType dx, const DefaultType dy,
        const double* juliaset_c, DefaultType escape_radius,
        const bool fast_mode, const bool juliaset, const bool lake,
        const int top_colors_outside, const int top_colors_lake,  const double* z_initial, double* input_array, const uint32_t array_size);
    
    void lyapunov_kernel_call(uint8_t* output, const int* array_top_colors_outside,
        const int* array_top_colors_lake, const char* exp, const size_t exp_size,
        const uint16_t width, const uint16_t height, const uint16_t max_iter,
        const DefaultType xmin, const DefaultType ymin, const DefaultType dx, const DefaultType dy, DefaultType complex_a, DefaultType complex_b,
        const DefaultType z_initial_j, const DefaultType z_initial_k, DefaultType escape_radius, 
        const int top_colors_outside, const int top_colors_lake, double* input_array, const uint32_t array_size);
    
    void newton_kernel_call(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
        const size_t exp_size, const uint16_t width, const uint16_t height, const uint16_t max_iter,
        const DefaultType xmin, const DefaultType ymin, const DefaultType dx, const DefaultType dy, const double* juliaset_c,
        const bool juliaset, const int top_colors_outside, const int top_colors_lake,
        const double* z_initial, const DefaultType newton_epsilon, double* input_array, const uint32_t array_size);

    EXPORT void fractal(uint8_t* output,
        const int* array_top_colors_outside,
        const int* array_top_colors_lake,
        const char* exp,
        const uint16_t width, const uint16_t height,
        const uint16_t max_iter,
        const double xmin, const double ymin,
        const double dx, const double dy,
        const double* juliaset_c,
        double escape_radius,
        const bool fast_mode, const bool juliaset,
        const bool lake, const bool ignore_it, std::atomic<bool>& stopFlag,
        const int top_colors_outside,
        const int top_colors_lake,
        const double* z_initial,
        double* input_array,
        const uint32_t array_size,
        const int mode)     // 2, 4, or 8
    {
        
        if (escape_radius == 0.0) escape_radius = 2.0;
        escape_radius *= escape_radius;

        size_t exp_size = std::strlen(exp);


        #ifdef USE_CUDA
            // --- GPU Implementation ---
            fractal_kernel_call(output, array_top_colors_outside, array_top_colors_lake, exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, juliaset_c, escape_radius, fast_mode, juliaset, lake, top_colors_outside, top_colors_lake, z_initial, input_array, array_size);
        #else

        // Dispatch based on dimension
        switch (mode) {
            case 2:
                runFractalCPU<2>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake, ignore_it, stopFlag,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            case 4:
                runFractalCPU<4>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake, ignore_it, stopFlag,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            case 8:
                runFractalCPU<8>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake, ignore_it, stopFlag,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            default:
                std::cerr << "Unsupported dimension: " << mode << std::endl;
                return;
        }

        #endif
    }


    EXPORT void lyapunov(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, double complex_a, double complex_b,
                    const double z_initial_j, const double z_initial_k, double escape_radius, 
                    const int top_colors_outside, const int top_colors_lake, double* input_array, const uint32_t array_size) {

        
        
        escape_radius = escape_radius == 0.0 ? 600.0 : escape_radius;
        
        size_t exp_size = strlen(exp);

        const DefaultType dx = (xmax - xmin) / width;
        const DefaultType dy = (ymax - ymin) / height;
        
        #ifdef USE_CUDA
            // --- GPU Implementation ---
            lyapunov_kernel_call(output, array_top_colors_outside, array_top_colors_lake,
                exp, exp_size,width, height, max_iter,
                xmin, ymin, dx, dy, complex_a, complex_b,
                z_initial_j, z_initial_k, escape_radius, 
                top_colors_outside, top_colors_lake, input_array, array_size);
        #else
            // --- CPU Implementation using OpenMP ---


        #endif
    }


    EXPORT void newton(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double* juliaset_c,
                    const bool juliaset, const int top_colors_outside, const int top_colors_lake,
                    const double* z_initial, 
                    const double newton_epsilon, double* input_array, const uint32_t array_size) {



        size_t exp_size = strlen(exp);

        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;

        #ifdef USE_CUDA
            // --- GPU Implementation ---
            newton_kernel_call(output, array_top_colors_outside, array_top_colors_lake, exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, juliaset_c, juliaset, top_colors_outside, top_colors_lake, z_initial, newton_epsilon, input_array, array_size);
        #else
            // --- CPU Implementation using OpenMP ---

    
            
        #endif
    }

    
    EXPORT void sandpile(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
        const uint16_t height, const uint32_t n_grains, const int top_colors_outside, const uint16_t max_grains = 3) {


        std::vector<uint32_t> sandpile(width*height,0);

        // Add grains to the center of the sandpile
        int half_height = height / 2;
        int half_width = width / 2;
        if (height % 2 == 0 && width % 2 == 0) {
            sandpile[half_height * width + half_width] = n_grains / 4;
            sandpile[(half_height - 1) * width + half_width] = n_grains / 4;
            sandpile[half_height * width + (half_width - 1)] = n_grains / 4;
            sandpile[(half_height - 1) * width + (half_width - 1)] = n_grains / 4;
        } else {
            sandpile[half_height * width + half_width] = n_grains;
        }

        bool unstable = true;
        while (unstable) {
            unstable = false;

            // Process each cell in the sandpile
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;
                    // Distribute grains if the number of grains in the cell is greater than max_grains
                    uint32_t grains_to_distribute = sandpile[idx];
                    if (grains_to_distribute > max_grains && grains_to_distribute > 3) {
                        uint32_t distribute = grains_to_distribute / 4;
                        // Distribute grains to neighboring cells
                        if (y > 0) {
                            sandpile[(y - 1) * width + x] += distribute;
                        }
                        if (y < height - 1) {
                            sandpile[(y + 1) * width + x] += distribute;
                        }
                        if (x > 0) {
                            sandpile[y * width + (x - 1)] += distribute;
                        }
                        if (x < width - 1) {
                            sandpile[y * width + (x + 1)] += distribute;
                        }
                        // Remove grains from the current cell
                        sandpile[idx] %= 4;
                        unstable = true;
                    }

                    // Compute the index for output (each cell corresponds to three bytes for RGB)
                    int out_index = idx * 3;
                    int it = array_top_colors_outside[sandpile[idx] % top_colors_outside];
                    output[out_index]     = static_cast<uint8_t>((it >> 16) & 0xFF); // R
                    output[out_index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);  // G
                    output[out_index + 2] = static_cast<uint8_t>(it & 0xFF);         // B
                }
            }
        }
    }
}


#endif