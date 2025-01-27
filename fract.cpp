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

#include "custom_quaternion.h"
#include "parser.h"


void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}


static constexpr double pi = 3.1415926535897932384626433832795028841971693993751;
static constexpr double phi =1.6180339887498948482045868343656381177203091798057;
static constexpr double e =  2.7182818284590452353602874713526624977572470937000;






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




void update_output(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const double temp,
                const uint16_t width, const uint16_t iteration, const uint16_t x, const uint16_t y,
                const double escape_radius, const int top_colors_outside, const int top_colors_lake, const bool lake, const bool lya) {

    int index = (y * width + x) * 3;
    int it;
    
    if (temp < escape_radius && lake) {
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


void setQuaternionValues(const bool juliaset, Quaternion& c, Quaternion& z,
                    const double c_real, const double c_imag,
                    const double r_part, const double i_part,
                    const double z_initial_r, const double z_initial_i,
                    const double quaternion_j = 0.0, const double quaternion_k = 0.0) {

        switch (static_cast<int>(juliaset)) {
            case 1:
                c = Quaternion(c_real, c_imag);
                z = Quaternion(r_part, i_part, quaternion_j, quaternion_k);
                break;
            case 0:
                c = Quaternion(r_part, i_part);
                z = Quaternion(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
                break;
        }
}

void update_pendulum_output(uint8_t* output, const int* array_top_colors_outside, const uint16_t width,
                            const uint16_t x, const uint16_t y, const int attractor_index, const int num_attractors) {

    int index = (y * width + x) * 3;
    int it = array_top_colors_outside[attractor_index % num_attractors];

    output[index] = static_cast<uint8_t>((it >> 16) & 0xFF);       // R
    output[index + 1] = static_cast<uint8_t>((it >> 8) & 0xFF);    // G
    output[index + 2] = static_cast<uint8_t>(it & 0xFF);           // B
}


void generate_attractors(Quaternion* attractors, int n) {
    if (n <= 0) return;

    Quaternion start_point(1.5, 0.0, 0.0, 0.0);
    double angle_step = 2 * pi / n;

    for (int i = 0; i < n; ++i) {
        double angle_in_radians = angle_step * i;

        double new_r = start_point.real * std::cos(angle_in_radians) - start_point.imag * std::sin(angle_in_radians);
        double new_i = start_point.real * std::sin(angle_in_radians) + start_point.imag * std::cos(angle_in_radians);
        
        attractors[i] =  Quaternion(new_r, new_i, start_point.j, start_point.k);
    }
}



std::vector<Quaternion> generate_lorenz_trajectory(const double sigma, const double rho, const double beta, const double dt,
                        const int max_iter, const std::string expression, const double z_initial_r, const double z_initial_i, const double quaternion_j, const double quaternion_k) {

    std::vector<Quaternion> trajectory(max_iter);
    int i = 0;

    Quaternion point(z_initial_r, z_initial_i, quaternion_j, quaternion_k);
    const std::map<std::string, std::function<Quaternion()>> variables = {
        {"z", [&point]() { return point; }},
        {"z_k", [&point]() { return point.k; }},
        {"phi", [&]() { return phi; }},
        {"pi", [&]() { return pi; }},
        {"e", [&]() { return e; }},
        {"It", [&]() { return Quaternion(i);   }},
        {"dx", [&]() { return sigma * (point.imag - point.real) * dt; }},
        {"dy", [&]() { return (point.real * (rho - point.j) - point.imag) * dt; }},
        {"dz", [&]() { return (point.real * point.imag - beta * point.j) * dt; }}
    };
    double myArray[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    uint32_t arraySize = 5;
    
    
    std::map<std::string, std::pair<double*, uint32_t>> arrays = {
        {"array", {myArray, arraySize}}
    };

    Parser parser(expression, variables, arrays);
    const auto ast = parser.parse();

    while (i < max_iter) {
        point += ast->evaluate();
        trajectory[i] = point;
        ++i;
    }

    return trajectory;
}



extern "C" {

    // void scale(const float* input_tensor, float* scaled_tensor, const int input_size,
    //                 const float new_min, const float new_max) {
    //     std::signal(SIGINT, signal_handler);
    //     float current_min = *std::min_element(input_tensor, input_tensor + input_size);
    //     const float current_max = *std::max_element(input_tensor, input_tensor + input_size);

    //     if (current_min == current_max){
    //         current_min -= 1;
    //     } 

    //     const float scale_factor = (new_max - new_min) / (current_max - current_min);
        
    //     for (int i = 0; i < input_size; ++i) {
    //         scaled_tensor[i] = (input_tensor[i] - current_min) * scale_factor + new_min;
    //     }

    // }



    void fractal(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double c_real, const double c_imag, double escape_radius,
                    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
                    const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i, 
                    double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);

        escape_radius = escape_radius == 0.0 ? 2.0 : escape_radius;

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        

        const std::string expression = std::string(exp);

        if ( (expression == "z*z+c" || expression == "pow(z,2)+c")  ) {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    Quaternion c, z;
                    
                    setQuaternionValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

                    uint16_t iteration = 0;
                    double temp = z.mag();
                    
    
                    while ( temp < escape_radius && iteration < max_iter) {
                        z = z*z+c;
                        temp = z.mag();
                        ++iteration;
                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, temp, width,
                        iteration, x, y, escape_radius, top_colors_outside, top_colors_lake, lake, false);
                }
            }

        } else {
            
            std::map<std::string, std::pair<double*, uint32_t>> arrays = {
                    {"array", std::make_pair(input_array, array_size)}
                };
            
            
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Quaternion z,c;
                uint16_t iteration = 0;
                int y = 0;
                const std::map<std::string, std::function<Quaternion()>> variables = {
                    {"z", [&z]() { return z; }},
                    {"c", [&c]() { return c; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"It", [&]() { return Quaternion(iteration);   }},
                    {"y", [&]() { return Quaternion(y, 0.0); }},
                    {"x", [&]() { return Quaternion(x, 0.0); }}
                };
                
                
                
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables, arrays);
                const auto ast = parser.parse();
    
    
                while (y < height) {
                    setQuaternionValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

                    iteration = 0;
                    double temp = z.mag();

    
                    while (temp < escape_radius && iteration < max_iter) {
                        z = (ast->evaluate());
                        temp = z.mag();
                        ++iteration;
                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, temp, width,
                        iteration, x, y, escape_radius, top_colors_outside, top_colors_lake, lake, false);
                    ++y;
                }
            }
       } 
    }
    
    
    void magnet(uint8_t* output, const int* array_top_colors_outside, const char* exp,
                const uint16_t width, const uint16_t height, const uint16_t max_iter,
                const double xmin, const double xmax, const double ymin,
                const double ymax, const double v_real, const double v_imag,
                double escape_radius, const double quaternion_j, const double quaternion_k,
                int n_points, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);

        escape_radius = escape_radius == 0.0 ? 9.0 : escape_radius;

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;


        const std::string expression = std::string(exp);

        n_points = n_points > 0 ? n_points : 2;
        std::vector<Quaternion> attractors(n_points);
    
        // Generate attractors
        generate_attractors(attractors.data(), n_points);
        
        if (expression == "z*z+c") {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {

                    Quaternion z(xmin + x * dx, ymin + y * dy, quaternion_j, quaternion_k);
                    Quaternion velocity(v_real, v_imag);
            
                    const double damping = 0.1;
                    const double r0 = 0.1;

                    const int num_attractors = attractors.size();
            
                    uint16_t iteration = 0;
                    double temp = 0;
                    int closest_attractor_index = -1;
                    double min_distance = std::numeric_limits<double>::max();
            
                    while (iteration < max_iter) {
                        Quaternion force(0);
            
                        for (int i = 0; i < num_attractors; ++i) {
                            Quaternion diff = attractors[i] - z;
                            double distance2 = diff.magSquared() + r0 * r0;
                            force += diff / distance2;

                            if (distance2 < min_distance) {
                                min_distance = distance2;
                                closest_attractor_index = i;
                            }
                        }

                        force -= velocity * damping;
                        velocity += force;
                        z += velocity;
            

                        temp = z.magSquared();
                        if (temp > escape_radius) break;
                        ++iteration;
                    }
            
                    update_pendulum_output(output, array_top_colors_outside, width, x, y, closest_attractor_index, num_attractors);
                }
            }



        } else {
            
            std::map<std::string, std::pair<double*, uint32_t>> arrays = {
                {"array", std::make_pair(input_array, array_size)}
            };

            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Quaternion z,velocity, force, diff;
                const double damping = 0.1;
                uint16_t iteration = 0;
                int y;
                const std::map<std::string, std::function<Quaternion()>> variables = {
                    {"z", [&z]() { return z; }},
                    {"v", [&velocity]() { return velocity; }},
                    {"f", [&force]() { return force; }},
                    {"diff", [&diff]() { return diff; }},
                    {"d", [&]() { return Quaternion(damping); }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"It", [&]() { return Quaternion(iteration);   }},
                    {"y", [&]() { return Quaternion(y, 0.0); }},
                    {"x", [&]() { return Quaternion(x, 0.0); }}
                };
                

    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables, arrays);
                const std::shared_ptr<ASTNode> ast = parser.parse();

                const int num_attractors = attractors.size();    
                const double r0 = 0.1;
                for (int y = 0; y < height; ++y) {
                    z = Quaternion(xmin + x * dx, ymin + y * dy, quaternion_j, quaternion_k);
                    velocity = Quaternion(v_real, v_imag);

                    iteration = 0;
                    double temp = 0;
                    int closest_attractor_index = -1;
                    double min_distance = std::numeric_limits<double>::max();
            
                    while (iteration < max_iter) {
                        force = Quaternion(0);
            
                        for (int i = 0; i < num_attractors; ++i) {
                            diff = attractors[i] - z;
                            double distance2 = diff.magSquared() + r0 * r0;
                            force += diff / distance2;

                            if (distance2 < min_distance) {
                                min_distance = distance2;
                                closest_attractor_index = i;
                            }
                        }

                        force -= velocity * damping;
                        velocity += force;
                        z = ast->evaluate();
            

                        temp = z.magSquared();
                        if (temp > escape_radius) break;
                        ++iteration;
                    }
            
                    update_pendulum_output(output, array_top_colors_outside, width, x, y, closest_attractor_index, num_attractors);
                }
            }
       } 
    }


    
    void lorenz(uint8_t* output, const int* array_top_colors_outside, const double angle,
                const char* exp,
                const uint16_t width, const uint16_t height, const int max_iter,
                const double xmin, const double xmax, const double ymin, const double ymax,
                const double zmin, const double zmax, const double sigma, const double rho, const double beta,
                const double dt, const int top_colors_outside, const int axis, const int point_size,
                const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i) {
    
        std::signal(SIGINT, signal_handler);
    

        const uint16_t max_wh = std::max(width,height);
        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;
        const double dz = (zmax - zmin) / max_wh;

        std::string expression = std::string(exp);
        if (expression == "z*z+c") {
            expression = "dx+dy*1i+dz*1j";
        }
    
        const std::vector<Quaternion> trajectory = generate_lorenz_trajectory(sigma, rho, beta, dt, max_iter,
                                                expression, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

        const double camera_position_z = zmin;
    
        // Initialize depth buffer
        std::vector<float> depthBuffer(width * height, 1e16);
    
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < max_iter; ++i) {
            Quaternion temp = trajectory[i];
            temp = temp.rotate_in_circle(Quaternion(angle * (pi / 180.0)), Quaternion(axis));
            if (temp.j < camera_position_z || temp.j > zmax) {
                continue;
            }

            double depth = (temp.j - zmin) / dz;
    
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


    void lyapunov(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, double complex_a, double complex_b,
                    const double quaternion_j, const double quaternion_k,
                    const int top_colors_outside, const int top_colors_lake, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);
        
        const double dx = (xmax - xmin) / width;
        const double dy = (ymax - ymin) / height;
        

        const std::string expression = std::string(exp);

        if ( expression == "z*z+c" ) {

            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    const double x = xmin + i * dx;
                    const double y = ymin + j * dy;
                    const Quaternion a(0.5 + x * 0.5, complex_a, quaternion_j, quaternion_k);
                    const Quaternion b(0.5 + y * 0.5, complex_b, quaternion_j, quaternion_k);
                    Quaternion l(0.0, 0.0);
                    Quaternion v(0.5, 0.0);
                    

                    int k = 0;
                    while (k < max_iter) {
                        if (k % 12 < 6) {
                            v = (b * v * (1.0 - v));
                            l += (((b * (1.0 - 2.0 * v)).c_mag()).log());
                        } else {
                            v = (a * v * (1.0 - v));
                            l += (((a * (1.0 - 2.0 * v)).c_mag()).log());
                        }
                        ++k;
                    }

                    update_output( output, array_top_colors_outside, array_top_colors_lake, l.mag(), width,
                        0, i, j, 0.0, top_colors_outside, top_colors_lake, false, true);
                }
            }

        } else {

            std::map<std::string, std::pair<double*, uint32_t>> arrays = {
                {"array", std::make_pair(input_array, array_size)}
            };

            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < width; ++i) {
                Quaternion l, v, temp;
                int k = 0;
                int j = 0;
                const std::map<std::string, std::function<Quaternion()>> variables = {
                    {"v", [&v]() { return v; }},
                    {"l", [&l]() { return l; }},
                    {"c", [&temp]() { return temp; }},
                    {"It", [&k]() { return Quaternion(k);   }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"x", [&]() { return Quaternion(i); }},
                    {"y", [&]() { return Quaternion(j);   }},
                };
                

                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables, arrays);
                const auto ast = parser.parse();
                while (j < height) {
                    const double x = xmin + i * dx;
                    const double y = ymin + j * dy;
                    const Quaternion a(0.5 + x * 0.5, complex_a, quaternion_j, quaternion_k);
                    const Quaternion b(0.5 + y * 0.5, complex_b, quaternion_j, quaternion_k);
                    l = Quaternion(0.0, 0.0);
                    v = Quaternion(0.5, 0.0);

                    k = 0;
                    while (k < max_iter) {
                        if (k % 12 < 6) {
                            v = b * v * (1.0 - v);
                            temp = b;
                            l += (ast->evaluate());
                        } else {
                            v = a * v * (1.0 - v);
                            temp = a;
                            l += (ast->evaluate());
                        }
                        ++k;
                    }
                    const double labs = l.mag();
                    update_output( output, array_top_colors_outside, array_top_colors_lake, labs, width,
                        0, i, j, 0.0, top_colors_outside, top_colors_lake, false, true);
                    
                    ++j;
                }
            }
        }
    }


    void newton(uint8_t* output, const int* array_top_colors_outside, const int* array_top_colors_lake, const char* exp,
                    const uint16_t width, const uint16_t height, const uint16_t max_iter,
                    const double xmin, const double xmax, const double ymin,
                    const double ymax, const double c_real, const double c_imag,
                    const bool juliaset, const bool lake, const int top_colors_outside, const int top_colors_lake,
                    const double quaternion_j, const double quaternion_k, const double z_initial_r, const double z_initial_i,
                    const double newton_epsilon, double* input_array, const uint32_t array_size) {

        std::signal(SIGINT, signal_handler);

        bool l = lake;
        l = !l;

        const double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        

        const std::string expression = std::string(exp);

        if ( (expression == "z*z*z-1+c" || expression == "pow(z,3)-1+c") ) {
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {

                for (int y = 0; y < height; ++y) {
                    
                    Quaternion c, z;
                    setQuaternionValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);

                    uint16_t iteration = 0;
                    double temp = z.mag();
                    
                    while (iteration < max_iter) {
                        

                        const Quaternion last_z = z;
                        const Quaternion znew = 3.0*z*z;
                        z = (z*z*z-1+c);
                        temp = z.mag();
                        
                        if ( temp < 1e-13 || temp > 1e300 ) break;
                        z = ( last_z - ( z/znew ));
                        
                        ++iteration;
                    }

                    update_output( output, array_top_colors_outside, array_top_colors_lake, 3.0, width,
                        iteration, x, y, 0.0, top_colors_outside, top_colors_lake, false, false);
                }
            }

        } else {
            
            std::map<std::string, std::pair<double*, uint32_t>> arrays = {
                {"array", std::make_pair(input_array, array_size)}
            };
            // const double q_epsilon = (quaternion_j != 0.0 || quaternion_k != 0.0) ? newton_epsilon : 0.0; 
            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < width; ++x) {
                Quaternion z,c;
                uint16_t iteration = 0;
                int y;
                const std::map<std::string, std::function<Quaternion()>> variables = {
                    {"z", [&z]() { return z; }},
                    {"c", [&c]() { return c; }},
                    {"phi", [&]() { return phi; }},
                    {"pi", [&]() { return pi; }},
                    {"e", [&]() { return e;   }},
                    {"It", [&]() { return Quaternion(iteration);   }},
                    {"y", [&]() { return Quaternion(y, 0.0); }},
                    {"x", [&]() { return Quaternion(x, 0.0); }}
                };
                
    
                //Parser parser(x % 2 < 1 ? expression : exp, variables);
                Parser parser( expression, variables, arrays);
                const auto ast = parser.parse();
    
    
                for (int y = 0; y < height; ++y) {
                    setQuaternionValues(juliaset, c, z, c_real, c_imag, xmin + x * dx,
                        ymin + y * dy, z_initial_r, z_initial_i, quaternion_j, quaternion_k);
                    
                    iteration = 0;
                    double temp = z.mag();
                    
                    while (iteration < max_iter) {

                        const Quaternion last_z = z;
                        const double h(newton_epsilon);
                        
                        z += h;
                        const Quaternion next_z = ast->evaluate();
                        z = last_z;
                        z = ast->evaluate();
                        
                        temp = z.mag();
                        
                        if ( temp < 1e-13 || temp > 1e300 ) break;
                        const Quaternion znew = ( next_z - z )/(h); 
                        z = last_z - ( z/znew );
                        
                        ++iteration;
                    }
                    update_output( output, array_top_colors_outside, array_top_colors_lake, 3.0, width,
                        iteration, x, y, 0.0, top_colors_outside, top_colors_lake, false, false);
                }
            }
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



