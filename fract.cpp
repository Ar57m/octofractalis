// fract.cpp
#include <omp.h>
#include <cmath>
#include <cstdint>
#include <cmath>
#include <vector>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <chrono>

#include "custom_complex.h"
#include "parser.h"


void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}


static constexpr double pi = 3.1415926535897932384626433832795028841971693993751;
static constexpr double phi =1.6180339887498948482045868343656381177203091798057;
static constexpr double e =  2.7182818284590452353602874713526624977572470937000;

double noNan(double value) {
    return ( std::abs(value) < 1e300) ? value : 0;
}




int current = 0;

void display_progress( int &current, const int total, const int iteration_interval) {
    static auto start_time = std::chrono::steady_clock::now();
    static int avg_it_per_sec = 0;
    static auto last_update_time = start_time;

    if (current % iteration_interval == 0 || current == total) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_since_last_update = now - last_update_time;

        if (elapsed_since_last_update.count() > 1.0 || current == total) {
            last_update_time = now;

            double progress = static_cast<double>(current) / total * 100.0;

            std::chrono::duration<double> elapsed = now - start_time;
            avg_it_per_sec = (avg_it_per_sec == 0) ? static_cast<int>(current / elapsed.count()) : static_cast<int>((avg_it_per_sec + current / elapsed.count()) / 2.0);

            int bar_width = 50;
            int pos = static_cast<int>(bar_width * progress / 100.0);

            std::cout << "[";
            for (int i = 0; i < bar_width; ++i) {
                if (i <= pos) std::cout << "=";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress) << "%  [ "
                      << avg_it_per_sec << " it/s; " << int((total - current) / avg_it_per_sec) << "s left ] \r";
            std::cout.flush();
        }
    }
    current++;
}




void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const double temp,
                const uint16_t width, const uint16_t iteration, const uint16_t x,
                const uint16_t y, const int top_colors_outside, const int top_colors_lake, const bool lake, const bool lya) {

    int index = (y * width + x) * 3;
    int it;
    
    if (temp < 2 && lake) {
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



void setComplexValues(const bool juliaset, Complex& c, Complex& z,
                    const double c_real, const double c_imag,
                    const double r_part, const double i_part,
                    const double z_initial_r, const double z_initial_i,
                    const double quaternion_j = 0.0, const double quaternion_k = 0.0) {

        switch (static_cast<int>(juliaset)) {
            case 1:
                c = Complex(c_real, c_imag);
                z = Complex(r_part, i_part, quaternion_j, quaternion_k);
                break;
            case 0:
                c = Complex(r_part, i_part);
                z = Complex(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
                break;
        }
}




extern "C" {
    void scale(const float* input_tensor, float* scaled_tensor, const int input_size,
                    const float new_min, const float new_max) {
        std::signal(SIGINT, signal_handler);
        float current_min = *std::min_element(input_tensor, input_tensor + input_size);
        const float current_max = *std::max_element(input_tensor, input_tensor + input_size);

        if (current_min == current_max){
            current_min -= 1;
        } 

        const float scale_factor = (new_max - new_min) / (current_max - current_min);
        
        for (int i = 0; i < input_size; ++i) {
            scaled_tensor[i] = (input_tensor[i] - current_min) * scale_factor + new_min;
        }

    }



    void fractal(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, double* failed_gen, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double c_real, const double c_imag,
                    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
                    const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i) {

        std::signal(SIGINT, signal_handler);

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        
        *failed_gen = *failed_gen == 0 ? 1 : 1;
        current = 0;

        const std::string expression = std::string(exp);

        if ( (expression == "rt*rt+rw" || expression == "pow(rt,2)+rw")  ) {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    Complex c, z;
                    
                    setComplexValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

                    uint16_t iteration = 0;
                    double temp = noNan(z.abs());
                    
    
                    while ( temp < 2 && iteration < max_iter) {
                        z = z*z+c;
                        temp = noNan(z.abs());
                        ++iteration;
                    }
                    *failed_gen = temp > *failed_gen ? temp : *failed_gen;
                    update_output( output, array_top_colors_outside, array_top_colors_lake, temp, width,
                        iteration, x, y, top_colors_outside, top_colors_lake, lake, false);
                }
                //display_progress( current, width, 80);
            }
            //std::cout << "\n";

        } else {
            
            int y;
            *failed_gen = 0.0;
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Complex z,c;
    
                const std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&z]() { return z; }},
                    {"rw", [&c]() { return c; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"y", [&]() { return Complex (y, 0.0); }},
                    {"x", [&]() { return Complex (x, 0.0); }}
                };
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                const auto ast = parser.parse();
    
    
                for (int y = 0; y < height; ++y) {
                    setComplexValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);
                        
                    uint16_t iteration = 0;
                    double temp = noNan(z.abs());
    
                    while (temp < 2 && iteration < max_iter) {
                        z = (ast->evaluate());
                        temp = noNan(z.abs());
                        ++iteration;
                    }
                    *failed_gen = temp > *failed_gen ? temp : *failed_gen;
                    update_output( output, array_top_colors_outside, array_top_colors_lake, temp, width,
                        iteration, x, y, top_colors_outside, top_colors_lake, lake, false);
                }
                //display_progress( current, width, 80);
            }
            //std::cout << "\n";
       } 
    }



    void lyapunov(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, double* failed_gen,const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, double complex_a, double complex_b,
                    const double quaternion_j, const double quaternion_k,
                    const int top_colors_outside, const int top_colors_lake) {

        std::signal(SIGINT, signal_handler);
        
        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;
        
        *failed_gen = *failed_gen == 0 ? 1 : 1;
        current = 0;

        const std::string expression = std::string(exp);

        if ( (expression == "rt*rt+rw" || expression == "pow(rt,2)+rw") ) {

            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    const double x = xmin + i * dx;
                    const double y = ymin + j * dy;
                    const Complex a(0.5 + x * 0.5, complex_a, quaternion_j, quaternion_k);
                    const Complex b(0.5 + y * 0.5, complex_b, quaternion_j, quaternion_k);
                    Complex l(0.0, 0.0);
                    Complex v(0.5, 0.0);

                    for (int k = 0; k < max_iter; ++k) {
                        if (k % 12 < 6) {
                            v = (b * v * (1.0 - v));
                            l += (((b * (1.0 - 2.0 * v)).c_abs()).log());
                        } else {
                            v = (a * v * (1.0 - v));
                            l += (((a * (1.0 - 2.0 * v)).c_abs()).log());
                        }
                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, l.abs(), width,
                        0, i, j, top_colors_outside, top_colors_lake, false, true);
                }
                //display_progress( current, width, 80);
            }
            //std::cout << "\n";

        } else {

            
            *failed_gen = 0.0;
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                Complex l, v, temp;
                const std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&v]() { return v; }},
                    {"rw", [&l]() { return l; }},
                    {"rk", [&temp]() { return temp; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                };

                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                const auto ast = parser.parse();
                for (int j = 0; j < height; ++j) {
                    const double x = xmin + i * dx;
                    const double y = ymin + j * dy;
                    const Complex a(0.5 + x * 0.5, complex_a, quaternion_j, quaternion_k);
                    const Complex b(0.5 + y * 0.5, complex_b, quaternion_j, quaternion_k);
                    l = Complex (0.0, 0.0);
                    v = Complex (0.5, 0.0);


                    for (int k = 0; k < max_iter; ++k) {
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            temp = b;
                            l += (ast->evaluate());
                        } else {
                            v = a * v * (1.0 - v);
                            temp = a;
                            l += (ast->evaluate());
                        }
                    }
                    const double labs = noNan(l.abs());
                    *failed_gen = labs > *failed_gen ? labs : *failed_gen;
                    update_output( output, array_top_colors_outside, array_top_colors_lake, labs, width,
                        0, i, j, top_colors_outside, top_colors_lake, false, true);
                    
                }
                // display_progress( current, width, 80);
            }
            // std::cout << "\n";
        }
    }


    void newton(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, double* failed_gen, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double c_real, const double c_imag,
                    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
                    const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i,
                    const double newton_epsilon) {

        std::signal(SIGINT, signal_handler);

        bool l = lake;
        l = !l;

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        
        *failed_gen = *failed_gen == 0 ? 1 : 1;
        current = 0;

        const std::string expression = std::string(exp);

        if ( (expression == "rt*rt*rt-1+rw" || expression == "pow(rt,3)-1+rw") ) {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {

                for (int y = 0; y < height; ++y) {
                    
                    Complex c, z;
                    setComplexValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

                    uint16_t iteration = 0;
                    double temp = noNan(z.abs());
                    
                    while (iteration < max_iter) {
                        

                        const Complex last_z = z;
                        const Complex znew = 3.0*z*z;
                        z = (z*z*z-1+c);
                        temp = noNan(z.abs());
                        
                        if ( temp < 1e-13 || temp > 1e300 ) break;
                        z = ( last_z - ( z/znew ));
                        
                        ++iteration;
                    }
                    *failed_gen = 3.0; //temp > *failed_gen ? temp : *failed_gen;
                    update_output( output, array_top_colors_outside, array_top_colors_lake, 3.0, width,
                        iteration, x, y, top_colors_outside, top_colors_lake, false, false);
                }
                // display_progress( current, width, 80);
            }
            // std::cout << "\n";

        } else {
            
            int y;
            *failed_gen = 0.0;
            const double q_epsilon = (quaternion_j != 0.0 || quaternion_k != 0.0) ? newton_epsilon : 0.0; 
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Complex z,c;
    
                const std::map<std::string, std::function<Complex()>> variables = {
                    {"rt", [&z]() { return z; }},
                    {"rw", [&c]() { return c; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"y", [&]() { return Complex (y, 0.0); }},
                    {"x", [&]() { return Complex (x, 0.0); }}
                };
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables);
                const auto ast = parser.parse();
    
    
                for (int y = 0; y < height; ++y) {
                    setComplexValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);
                    
                    uint16_t iteration = 0;
                    double temp = noNan(z.abs());
                    
                    while (iteration < max_iter) {

                        const Complex last_z = z;
                        const Complex h(newton_epsilon, newton_epsilon, q_epsilon, q_epsilon);
                        
                        z += h;
                        const Complex next_z = ast->evaluate();
                        z = last_z;
                        z = ast->evaluate();
                        
                        temp = noNan(z.abs());
                        
                        if ( temp < 1e-13 || temp > 1e300 ) break;
                        const Complex znew = ( next_z - z )/(h); 
                        z = last_z - ( z/znew );
                        
                        ++iteration;
                    }
                    *failed_gen = 3.0; //temp > *failed_gen ? temp : *failed_gen;
                    update_output( output, array_top_colors_outside, array_top_colors_lake, 3.0, width,
                        iteration, x, y, top_colors_outside, top_colors_lake, false, false);
                }

                // display_progress( current, width, 80);
            }
            // std::cout << "\n";
       } 
    }

    void sandpile(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
                const uint16_t height, const uint32_t n_grains, const int top_colors_outside,const uint16_t max_grains=3) {
            std::signal(SIGINT, signal_handler);
            std::vector<std::vector<uint32_t>> sandpile(height, std::vector<uint32_t>(width, 0));
        
        // Add grains to the center of the sandpile
        int half_height = height / 2;
        int half_width = width / 2;
        if (height % 2 == 0 && width % 2 == 0) {
            sandpile[half_height][half_width] = n_grains / 4;
            sandpile[half_height - 1][half_width] = n_grains / 4;
            sandpile[half_height][half_width - 1] = n_grains / 4;
            sandpile[half_height - 1][half_width - 1] = n_grains / 4;
        } else {
            sandpile[half_height][half_width] = n_grains;
        }


        
            bool unstable = true;
            while (unstable) {
                unstable = false;
        
                // Process each cell in the sandpile
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        // Distribute grains if the number of grains in the cell is greater than max_grains
                        uint32_t grains_to_distribute = sandpile[y][x]; 
                        if (grains_to_distribute > max_grains &&  grains_to_distribute > 3) {
                            grains_to_distribute /= 4;
                            // Distribute grains to neighboring cells
                            if (y > 0) sandpile[y-1][x] += grains_to_distribute;
                            if (y < height-1) sandpile[y+1][x] += grains_to_distribute;
                            if (x > 0) sandpile[y][x-1] += grains_to_distribute;
                            if (x < width-1) sandpile[y][x+1] += grains_to_distribute;
        
                            // Remove grains from current cell
                            sandpile[y][x] %= 4;
                            unstable = true;
                        }
                        int index = (y * width + x) * 3;
                        int it = array_top_colors_outside[sandpile[y][x] % top_colors_outside];
                        output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
                        output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
                        output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
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



