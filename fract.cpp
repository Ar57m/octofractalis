// fract.cpp
#include <omp.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
// #include <chrono>
#include <cstring>

#ifndef OCTO
    #include "custom_quaternion.h"
#else
    #include "custom_octonion.h"
#endif

#ifndef USE_CUDA
#include "parser.h"
QuaternionOrOctonion pi(3.1415926535897932384626433832795028841971693993751);
QuaternionOrOctonion phi(1.6180339887498948482045868343656381177203091798057);
QuaternionOrOctonion e(2.7182818284590452353602874713526624977572470937000);
#endif

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif


void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}









// void display_progress( int &current, const int total, const int iteration_interval) {
//     static auto start_time = std::chrono::steady_clock::now();
//     static int avg_it_per_sec = 0;
//     static auto last_update_time = start_time;

//     if (current % iteration_interval == 0 || current == total) {
//         auto now = std::chrono::steady_clock::now();
//         std::chrono::duration<double> elapsed_since_last_update = now - last_update_time;

//         if (elapsed_since_last_update.count() > 1.0 || current == total) {
//             last_update_time = now;

//             double progress = static_cast<double>(current) / total * 100.0;

//             std::chrono::duration<double> elapsed = now - start_time;
//             avg_it_per_sec = (avg_it_per_sec == 0) ? static_cast<int>(current / elapsed.count()) : static_cast<int>((avg_it_per_sec + current / elapsed.count()) / 2.0);

//             int bar_width = 50;
//             int pos = static_cast<int>(bar_width * progress / 100.0);

//             std::cout << "[";
//             for (int i = 0; i < bar_width; ++i) {
//                 if (i <= pos) std::cout << "=";
//                 else std::cout << " ";
//             }
//             std::cout << "] " << int(progress) << "%  [ "
//                       << avg_it_per_sec << " it/s; " << int((total - current) / avg_it_per_sec) << "s left ] \r";
//             std::cout.flush();
//         }
//     }
//     current++;
// }




void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const DefaultType temp,
                const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y,
                const bool not_escaped, const int top_colors_outside, const int top_colors_lake, const bool lake, const bool lya) {

    const int index = (y * width + x) * 3;
    int it = 0;
    
    if (not_escaped && lake) {
        it = array_top_colors_lake[static_cast<int>(std::round((temp / (temp + 1.0)) * top_colors_lake))];
    } else if (lya) {
        it = array_top_colors_outside[static_cast<int>((std::round((temp / (temp + top_colors_outside / 10.0)) * top_colors_outside)))];
    } else {
        it = array_top_colors_outside[iteration % (top_colors_outside+1)];
    }


    output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
}



void drawFilledCircle(uint8_t* array, float* depthBuffer, const int rows, const int cols, 
                      const int centerX, const int centerY, 
                      const int radius, const float depth, const int color) {

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            int newX = centerX + x;
            int newY = centerY + y;

            if (x * x + y * y <= radius * radius && 
                newX >= 0 && newX < cols && 
                newY >= 0 && newY < rows) {

                int pixelIndex = newY * cols + newX;
                int colorIndex = pixelIndex * 3;

                // Depth check
                if (depth < depthBuffer[pixelIndex]) {
                    // Update depth buffer
                    depthBuffer[pixelIndex] = depth;

                    // Draw the pixel
                    array[colorIndex] = static_cast<uint8_t>((color >> 16) & 0xFF);     // R
                    array[colorIndex + 1] = static_cast<uint8_t>((color >> 8) & 0xFF);  // G
                    array[colorIndex + 2] = static_cast<uint8_t>(color & 0xFF);         // B
                }
            }
        }
    }
}


void setQuaternionOrOctonionValues(const bool juliaset, QuaternionOrOctonion& c, QuaternionOrOctonion& z,
                    const double* juliaset_c, const DefaultType r_part, const DefaultType i_part,
                    const double* z_initial
                ) {



#ifndef OCTO
    if (juliaset) {
        c = QuaternionOrOctonion(juliaset_c[0], juliaset_c[1], juliaset_c[2], juliaset_c[3]);
        z = QuaternionOrOctonion(r_part, i_part, z_initial[2], z_initial[3]);
    } else {
        c = QuaternionOrOctonion(r_part, i_part);
        z = QuaternionOrOctonion(z_initial[0], z_initial[1], z_initial[2], z_initial[3]);
    }
#else
    if (juliaset) {
        c = QuaternionOrOctonion(juliaset_c[0], juliaset_c[1], juliaset_c[2], juliaset_c[3], juliaset_c[4], juliaset_c[5], juliaset_c[6], juliaset_c[7]);
        z = QuaternionOrOctonion(r_part, i_part, z_initial[2], z_initial[3], z_initial[4], z_initial[5], z_initial[6], z_initial[7]);
    } else {
        c = QuaternionOrOctonion(r_part, i_part);
        z = QuaternionOrOctonion(z_initial[0], z_initial[1], z_initial[2], z_initial[3], z_initial[4], z_initial[5], z_initial[6], z_initial[7]);
    }
#endif

}

void update_pendulum_output(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
                            const uint16_t x, const uint16_t y, const int attractor_index, const int num_attractors) {

    int index = (y * width + x) * 3;
    int it = array_top_colors_outside[attractor_index % num_attractors];

    output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
}


void generate_attractors(QuaternionOrOctonion* attractors, int n) {
    if (n <= 0) return;

    DefaultType angle_step = 2 * 3.1415926535897932384626433832795028841971693993751 / n;

    for (int i = 0; i < n; ++i) {


        DefaultType angle_in_radians = angle_step * i;

        DefaultType new_r = 1.5 * std::cos(angle_in_radians);
        DefaultType new_i = 1.5 * std::sin(angle_in_radians);
        
        attractors[i] =  QuaternionOrOctonion(new_r, new_i);
    }
}

#ifndef USE_CUDA

void generate_lorenz_trajectory(QuaternionOrOctonion* trajectory, const DefaultType sigma, const DefaultType rho, const DefaultType beta, const DefaultType dt,
                        const int max_iter, const char* expression, const size_t exp_size, const double* z_initial, double* input_array, const uint32_t array_size) {
    

    QuaternionOrOctonion pi(3.1415926535897932384626433832795028841971693993751);
    QuaternionOrOctonion phi(1.6180339887498948482045868343656381177203091798057);
    QuaternionOrOctonion e(2.7182818284590452353602874713526624977572470937000);
#ifndef OCTO
    QuaternionOrOctonion point(z_initial[0], z_initial[1], z_initial[2], z_initial[3]);
#else
    QuaternionOrOctonion point(z_initial[0], z_initial[1], z_initial[2], z_initial[3], z_initial[4], z_initial[5], z_initial[6], z_initial[7]);
#endif
    
    QuaternionOrOctonion dx = 0.0;
    QuaternionOrOctonion dy = 0.0;
    QuaternionOrOctonion dz = 0.0;
    QuaternionOrOctonion it_quat = 0.0;
    const QuaternionOrOctonion dt_q = 0.0;

    ArrayEntry arrEntries[1] = {
        {"array", input_array, array_size}
    };
    const size_t numArrays = 1;

    VariableEntry varEntries[9] = {
        {"z", &point},
        {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
        {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
        {"e", const_cast<QuaternionOrOctonion*>(&e)},
        {"it", &it_quat},
        {"dx", &dx},
        {"dy", &dy},
        {"dz", &dz},
        {"dt", const_cast<QuaternionOrOctonion*>(&dt_q)}
    };
    
    const size_t numVars = 9;
    Parser parser(expression, exp_size, varEntries, numVars, arrEntries, numArrays);
    const ASTNode* ast = parser.parse(); // dx+dy*1i+dz*1j

    for (int i = 0; i < max_iter; ++i) {
        it_quat = static_cast<DefaultType>(i);
        dx = sigma * (point.imag - point.real) * dt;
        dy = (point.real * (rho - point.j) - point.imag) * dt;
        dz = (point.real * point.imag - beta * point.j) * dt;
        point += ast->evaluate();
        trajectory[i] = point;
    }


}

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

    void magnet_kernel_call(uint8_t* output, const int* array_top_colors_outside, const QuaternionOrOctonion* attractors, const char* exp,
        const size_t exp_size, const uint16_t width, const uint16_t height, const uint16_t max_iter,
        const DefaultType xmin, const DefaultType ymin, const DefaultType dx, const DefaultType dy,
        const DefaultType v_real, const DefaultType v_imag,
        DefaultType escape_radius, const DefaultType z_initial_j, const DefaultType z_initial_k,
        const bool fast_mode, int n_points, double* input_array, const uint32_t array_size);

    void generate_lorenz_trajectory_kernel(QuaternionOrOctonion* trajectory, const DefaultType sigma, const DefaultType rho, const DefaultType beta, const DefaultType dt,
        const int max_iter, const char* expression, const size_t exp_size, const double* z_initial, double* input_array, const uint32_t array_size);





    EXPORT void fractal(uint8_t* output, const int* array_top_colors_outside,
        const int* array_top_colors_lake, const char* exp,
        const uint16_t width, const uint16_t height, const uint16_t max_iter,
        const double xmin, const double xmax, const double ymin, const double ymax,
        const double* juliaset_c, double escape_radius,
        const bool fast_mode, const bool juliaset, const bool lake,
        const int top_colors_outside, const int top_colors_lake, const double* z_initial,
        double* input_array, const uint32_t array_size)
    {
    
    std::signal(SIGINT, signal_handler);
    if (escape_radius == 0.0) escape_radius = 2.0;

    size_t exp_size = strlen(exp);

    const DefaultType dx = (xmax - xmin) / width;
    const DefaultType dy = (ymax - ymin) / height;
    
    #ifdef USE_CUDA
        // --- GPU Implementation ---

        fractal_kernel_call(output, array_top_colors_outside, array_top_colors_lake, exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, juliaset_c, escape_radius, fast_mode, juliaset, lake, top_colors_outside, top_colors_lake, z_initial, input_array, array_size);
    #else
        // --- CPU Implementation using OpenMP ---

        ArrayEntry arrEntries[1] = {
            {"array", input_array, array_size}
        };
        const size_t numArrays = 1;

        #pragma omp parallel for schedule(dynamic)
        for (int x = 0; x < width; ++x) {
            QuaternionOrOctonion z , c, last_it_z;
            QuaternionOrOctonion it_quat(0.0);
            QuaternionOrOctonion x_quat(static_cast<DefaultType>(x));
            QuaternionOrOctonion y_quat(0.0);
            int y = 0;

            VariableEntry varEntries[8] = {
                {"z", &z},
                {"c", &c},
                {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
                {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
                {"e", const_cast<QuaternionOrOctonion*>(&e)},
                {"it", &it_quat},
                {"y", &y_quat},
                {"x", &x_quat}
            };
            const size_t numVars = 8;
            Parser parser(exp, exp_size, varEntries, numVars, arrEntries, numArrays);
            const ASTNode* ast = parser.parse();

            while (y < height) {
                y_quat = (static_cast<DefaultType>(y));
                setQuaternionOrOctonionValues(juliaset, c, z, juliaset_c, xmin + x * dx,
                    ymin + y * dy, z_initial);

                uint16_t iteration = 0;
                DefaultType temp = 0.0;
                bool not_escaped = true;
                

                while ( not_escaped && iteration < max_iter) {
                    it_quat = QuaternionOrOctonion(static_cast<DefaultType>(iteration));
                    last_it_z = (ast->evaluate());
                    if (last_it_z == z && fast_mode) break;
                    z = last_it_z;
                    temp = z.mag();
                    ++iteration;
                    not_escaped = temp < escape_radius;
                }
                update_output( output, array_top_colors_outside, array_top_colors_lake, temp, width,
                    iteration, x, y, not_escaped, top_colors_outside, top_colors_lake, lake, false);
                ++y;
            }
        }
    #endif
    }
    
    
    EXPORT void magnet(uint8_t* output, const int* array_top_colors_outside, const char* exp,
                const uint16_t width, const uint16_t height, const uint16_t max_iter,
                const double xmin, const double xmax, const double ymin,
                const double ymax, const double v_real, const double v_imag,
                double escape_radius, const double z_initial_j, const double z_initial_k,
                const bool fast_mode, int n_points, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);

        escape_radius = escape_radius == 0.0 ? 9.0 : escape_radius;

        n_points = n_points > 1 ? n_points : 2;
        std::vector<QuaternionOrOctonion> attractors(n_points);
    
        // Generate attractors
        generate_attractors(attractors.data(), n_points);
        
        size_t exp_size = strlen(exp);

        const DefaultType dx = (xmax - xmin) / width;
        const DefaultType dy = (ymax - ymin) / height;

        #ifdef USE_CUDA
            // --- GPU Implementation ---
            magnet_kernel_call(output, array_top_colors_outside, attractors.data(), exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, v_real, v_imag, escape_radius, z_initial_j, z_initial_k, fast_mode, n_points, input_array, array_size);

        #else
            // --- CPU Implementation using OpenMP ---

    
            ArrayEntry arrEntries[1] = {
                {"array", input_array, array_size}
            };

            const size_t numArrays = 1;
    
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                QuaternionOrOctonion z, velocity, force, dif;
                const DefaultType damping = 0.1;
                QuaternionOrOctonion it_quat(0.0);
                QuaternionOrOctonion x_quat(static_cast<DefaultType>(x));
                QuaternionOrOctonion y_quat(0.0);
                int y = 0;
    
                VariableEntry varEntries[10] = {
                    {"z", &z},
                    {"v", &velocity},
                    {"f", &force},
                    {"dif", &dif},
                    {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
                    {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
                    {"e", const_cast<QuaternionOrOctonion*>(&e)},
                    {"it", &it_quat},
                    {"y", &y_quat},
                    {"x", &x_quat}
                };

                const size_t numVars = 10;
                Parser parser(exp, exp_size, varEntries, numVars, arrEntries, numArrays);
                const ASTNode* ast = parser.parse();
                const int num_attractors = attractors.size();    
                const DefaultType r0 = 0.1;
    
                while (y < height) {
                    y_quat = (static_cast<DefaultType>(y));
                    QuaternionOrOctonion last_it_z;
                    z = QuaternionOrOctonion(xmin + x * dx, ymin + y * dy, z_initial_j, z_initial_k);
                    velocity = QuaternionOrOctonion(v_real, v_imag);

                    uint16_t iteration = 0;
                    DefaultType temp = 0;
                    int closest_attractor_index = -1;
                    DefaultType min_distance = Max_flt;
                    
    
                    while (iteration < max_iter) {
                        it_quat = QuaternionOrOctonion(static_cast<DefaultType>(iteration));
                        force = QuaternionOrOctonion(0);
            
                        for (int i = 0; i < num_attractors; ++i) {
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
                    update_pendulum_output(output, array_top_colors_outside, width, x, y, closest_attractor_index, num_attractors);
                    ++y;
                }
            }
        #endif
    }


    
    EXPORT void lorenz(uint8_t* output, const int* array_top_colors_outside, const double angle,
                const char* exp, const uint16_t width, const uint16_t height, const int max_iter,
                const double xmin, const double xmax, const double ymin, const double ymax,
                const double zmin, const double zmax, const double sigma, const double rho, const double beta,
                const double dt, const int top_colors_outside, const int axis, const int point_size,
                const double* z_initial, double* input_array, const uint32_t array_size) {
        std::signal(SIGINT, signal_handler);
    

        const uint16_t max_wh = std::max(width,height);
        const DefaultType dx = (xmax - xmin) / width;
        const DefaultType dy = (ymax - ymin) / height;
        const DefaultType dz = (zmax - zmin) / max_wh;

        size_t exp_size = strlen(exp);

        std::vector<QuaternionOrOctonion> trajectory(max_iter);

        #ifdef USE_CUDA
        generate_lorenz_trajectory_kernel(trajectory.data(), sigma, rho, beta, dt, max_iter,
            exp, exp_size, z_initial, input_array, array_size); // dx+dy*1i+dz*1j
        
        #else
        generate_lorenz_trajectory(trajectory.data(), sigma, rho, beta, dt, max_iter,
            exp, exp_size, z_initial, input_array, array_size); // dx+dy*1i+dz*1j
        
        #endif

        const DefaultType camera_position_z = zmin;
    
        // Initialize depth buffer
        std::vector<float> depthBuffer(width * height, 1e16);
    
        //#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < max_iter; ++i) {
            QuaternionOrOctonion temp = trajectory[i];
            temp = temp.rotate_in_circle(QuaternionOrOctonion(angle * (3.1415926535897932384626433832795028841971693993751 / 180.0)), QuaternionOrOctonion(axis));
            if (temp.j < camera_position_z || temp.j > zmax) {
                continue;
            }

            DefaultType depth = (temp.j - zmin) / dz;
    
            int pixel_x = static_cast<int>((temp.real - xmin) / dx);
            int pixel_y = static_cast<int>((temp.imag - ymin) / dy);
            depth =  ((depth / max_wh) * point_size);
    
            int scale_factor = static_cast<int>(point_size - depth);
    
            if (pixel_x >= 0 && pixel_x < width && pixel_y >= 0 && pixel_y < height) {
                drawFilledCircle(output, depthBuffer.data(), height, width, pixel_x, pixel_y, scale_factor,
                                 depth, array_top_colors_outside[i % (top_colors_outside + 1)]);
            }
        }
    }


    EXPORT void lyapunov(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, double complex_a, double complex_b,
                    const double z_initial_j, const double z_initial_k, double escape_radius, 
                    const int top_colors_outside, const int top_colors_lake, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);
        
        
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

            ArrayEntry arrEntries[1] = {
                {"array", input_array, array_size}
            };
            const size_t numArrays = 1;
    
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                QuaternionOrOctonion v, p, temp;
                QuaternionOrOctonion it_quat(0.0);
                QuaternionOrOctonion x_quat(static_cast<DefaultType>(x));
                QuaternionOrOctonion y_quat(0.0);
                int y = 0;
                int k = 0;
                
    
                VariableEntry varEntries[9] = {
                    {"v", &v},
                    {"p", &p},
                    {"c", &temp},
                    {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
                    {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
                    {"e", const_cast<QuaternionOrOctonion*>(&e)},
                    {"it", &it_quat},
                    {"y", &y_quat},
                    {"x", &x_quat}
                };

                const size_t numVars = 9;
                Parser parser(exp, exp_size, varEntries, numVars, arrEntries, numArrays);
                const ASTNode* ast = parser.parse();
    
                while (y < height) {
                    const QuaternionOrOctonion a(0.5 + (xmin + x * dx) * 0.5, complex_a, z_initial_j, z_initial_k);
                    const QuaternionOrOctonion b(0.5 + (ymin + y * dy) * 0.5, complex_b, z_initial_j, z_initial_k);
                    p = QuaternionOrOctonion(0.0, 0.0);
                    v = QuaternionOrOctonion(0.5, 0.0);

                    k = 0;
                    y_quat = (static_cast<DefaultType>(y));


                    double p_mag = 0.0;

                    while (k < max_iter) {
                        it_quat = QuaternionOrOctonion(static_cast<double>(k));
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            temp = b;
                            p += (ast->evaluate());
                        } else { //     log(mag(c*(1-2*v)))
                            v = a * v * (1.0 - v);
                            temp = a;
                            p += (ast->evaluate());
                        }
                        p_mag = p.mag();
                        if (p_mag > escape_radius) break;
                        ++k;
                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, p_mag, width,
                        0, x, y, false, top_colors_outside, top_colors_lake, false, true);
                    ++y;
                }
            }
        #endif
    }


    EXPORT void newton(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double* juliaset_c,
                    const bool juliaset, const int top_colors_outside, const int top_colors_lake,
                    const double* z_initial, 
                    const double newton_epsilon, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);


        size_t exp_size = strlen(exp);

        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;

        #ifdef USE_CUDA
            // --- GPU Implementation ---
            newton_kernel_call(output, array_top_colors_outside, array_top_colors_lake, exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, juliaset_c, juliaset, top_colors_outside, top_colors_lake, z_initial, newton_epsilon, input_array, array_size);
        #else
            // --- CPU Implementation using OpenMP ---

    
            ArrayEntry arrEntries[1] = {
                {"array", input_array, array_size}
            };
            const size_t numArrays = 1;
    
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                QuaternionOrOctonion z , c;
                QuaternionOrOctonion it_quat(0.0);
                QuaternionOrOctonion x_quat(static_cast<double>(x));
                QuaternionOrOctonion y_quat(0.0);
                int y = 0;
    
                VariableEntry varEntries[8] = {
                    {"z", &z},
                    {"c", &c},
                    {"phi", const_cast<QuaternionOrOctonion*>(&phi)},
                    {"pi", const_cast<QuaternionOrOctonion*>(&pi)},
                    {"e", const_cast<QuaternionOrOctonion*>(&e)},
                    {"it", &it_quat},
                    {"y", &y_quat},
                    {"x", &x_quat}
                };
                const size_t numVars = 8;
                Parser parser(exp, exp_size, varEntries, numVars, arrEntries, numArrays);
                const ASTNode* ast = parser.parse();
    
                while (y < height) {
                    y_quat = (static_cast<double>(y));
                    setQuaternionOrOctonionValues(juliaset, c, z, juliaset_c, xmin + x * dx,
                       ymin + y * dy, z_initial);
    
                    uint16_t iteration = 0;
                    double temp = 0.0;
                    
    
                    while (iteration < max_iter) {
                        it_quat = QuaternionOrOctonion(static_cast<double>(iteration));

                        const QuaternionOrOctonion last_z = z;
                        const double h(newton_epsilon);
                        
                        z += h;
                        const QuaternionOrOctonion next_z = ast->evaluate();
                        z = last_z;
                        z = ast->evaluate();
                        
                        temp = z.mag();
                        
                        if ( temp < 1e-13 ) break;
                        const QuaternionOrOctonion znew = ( next_z - z )/(h); 
                        z = last_z - ( z/znew );
                        ++iteration;

                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, 3.0, width,
                        iteration, x, y, false, top_colors_outside, top_colors_lake, false, false);
                    
                    ++y;
                }
            }
        #endif
    }

    
    EXPORT void sandpile(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
        const uint16_t height, const uint32_t n_grains, const int top_colors_outside, const uint16_t max_grains = 3) {

        std::signal(SIGINT, signal_handler);

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




    // void process_array(uint32_t* input_array, uint8_t* output_array, const uint16_t width,
    //                 const uint16_t height, const double max_value, const uint16_t batch_size,
    //                 const double npmax) {
    //     std::signal(SIGINT, signal_handler);
    //     // Iterate over each batch
        
    //     #pragma omp parallel for schedule(dynamic)
    //     for(int i = 0; i < width * height; i += batch_size) {
    //         // Iterate over each value in the batch
    //         for(int j = i; j < i + batch_size && j < width * height; j++) {
    //             // Convert the value to double and scale it
    //             const double value = (static_cast<double>((input_array[j])) / npmax) * max_value;

    //             // Round to nearest integer
    //             const uint32_t rounded_value = static_cast<uint32_t>(std::round(value));
                
    //             // Separate RGB channels
    //             output_array[j * 3] = (rounded_value >> 16) & 0xFF;
    //             output_array[j * 3 + 1] = (rounded_value >> 8) & 0xFF;
    //             output_array[j * 3 + 2] = rounded_value & 0xFF;
    //         }
    //     }
    // }
}



