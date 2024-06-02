// fract.cpp
#include <omp.h>
#include <math.h>
#include <cstdint>
#include <cmath>
#include <vector>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <algorithm>


void signal_handler(int signal) {
    std::cout << "(Ctrl+C)" << std::endl;
    std::exit(signal);
}

double tanh_approx(double x) {
    double x2 = x * x;
    double x4 = x2 * x2;
    return x * (21.0+9.0 * x2 + 2.0 * x4)/(20.5 + 18.0 * x2 + 3.0 * x4);
}
double sin_approx(double x) {
    double x2 = x * x;
    return x * (1 - x2 / 6 * (1 - x2 / 20));
}

double cos_approx(double x) {
    double x2 = x * x;
    return 1 - x2 / 2 * (1 - x2 / 12);
}

double tan_approx(double x) {
    return x * (1 + x * x / 3);
}

double sinh_approx(double x) {
    double x2 = x * x;
    return x * (1 + x2 / 6 * (1 + x2 / 20));
}

double cosh_approx(double x) {
    double x2 = x * x;
    return 1 + x2 / 2 * (1 + x2 / 12);
}

double log_approx(double x) {
    if (x <= 0) {
        return 0;
    }
    double y = (x - 1) / (x + 1);
    double y2 = y * y;

    if (x <= 0.78) {
        y2 = 2 * y * (1 + y2 / 3 * (1 + y2 / 5 * (1 + y2 / 7 * (1 + y2 / 9))));
    } else {
        y2 = ((-1) / (x + 0.23)) + 0.7;
    }
    return y2;
}


double log10_approx(double x) {
    if (x <= 0.715) {
            return log_approx(x) / 2.302585093;
    } else {
            double y = (x - 1) / (x + 1);
            double y2 = y * y;
            return (2 * y * (1 + y2 / 3 * (1 + y2 / 5 * (1 + y2 / 7 * (1 + y2 / 9))))) / 2.302585093;
    }

}


double noNan(double value) {
    if (std::isnan(value) || std::isinf(value)) {
        return 0;
    } else {
        return value;
    }
}

double pi = M_PI;    //3.141592653589793;
double e = M_E;       //2.718281828459045;






extern "C" {
    void scale(const float* input_tensor, float* scaled_tensor, int input_size, float new_min, float new_max) {
        std::signal(SIGINT, signal_handler);
        float current_min = *std::min_element(input_tensor, input_tensor + input_size);
        float current_max = *std::max_element(input_tensor, input_tensor + input_size);

        if (current_min == current_max){
            current_min -= 1;
        } 

        float scale_factor = (new_max - new_min) / (current_max - current_min);
        
        for (int i = 0; i < input_size; ++i) {
            scaled_tensor[i] = (input_tensor[i] - current_min) * scale_factor + new_min;
        }

    }



    void fractal(uint16_t* output, uint16_t width, uint16_t height, uint16_t max_iter, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, bool lake=false) {

        std::signal(SIGINT, signal_handler);

        double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;

        #pragma omp parallel for schedule(dynamic)
        for (int x = 0; x < width; ++x) {
            double r_part = 0;
            r_part = xmin + x * dx;
            for (int y = 0; y < height; ++y) {
                double i_part = ymin + y * dy;
                double z_real = 0;
                double z_imag = 0;
                double c_real = 0;
                double c_imag = 0;
                c_real = r_part;
                c_imag = i_part;
                uint16_t iteration = 0;
                

                while (z_real * z_real + z_imag * z_imag < 4 && iteration < max_iter) {
                    double z2_real = z_real * z_real - z_imag * z_imag + c_real;
                    double z2_imag = 2 * z_real * z_imag + c_imag;
                    double temp_real = (z2_real);
                    double temp_imag = (z2_imag);

                    z_real = temp_real;
                    z_imag = temp_imag;
                    ++iteration;
                }
                double temp = z_real * z_real + z_imag * z_imag;
                if (temp < 4 && lake) {
                    output[y *width + x] = temp < 0 ? static_cast<uint16_t>(std::round((-temp/(-temp+1))*max_iter)) : static_cast<uint16_t>(std::round((temp/(temp+1))*max_iter));
                } else {
                    output[y * width + x] = iteration;
                }
            }
        }
    }




    void lyapunov(uint16_t* output, uint16_t width, uint16_t height, uint16_t max_iter, double xmin=3.4, double xmax=4.0, double ymin=2.5, double ymax=3.4) {
    
        std::signal(SIGINT, signal_handler);
    
        double dx = (xmax - xmin) / width;
        double dy = (ymax - ymin) / height;

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                double x = xmin + j * dx;
                double y = ymin + i * dy;
                double a = 0.5 + x * 0.5;
                double b = 0.5 + y * 0.5;
                double l = 0.0;
                double v = 0.5;
    
                for (int k = 0; k < max_iter; k += 6) {
                    for (int lote = 0; lote < 6; ++lote) {
                        if ((k +lote) % 12 < 6) {
                            v = b * v * (1 - v);
                            l += noNan(log(fabs(b * (1 - 2 * v))));
                        } else {
                            v = a * v * (1 - v);
                            l += noNan(log(fabs(a * (1 - 2 * v))));
                        }
                    }
                }
                output[i * width + j] = l < 0 ? static_cast<uint16_t>((std::round((-l/(-l+1))*max_iter))) : static_cast<uint16_t>((std::round((l/(l+1))*max_iter)));
            }
        }
    }
    



    void sandpile(uint8_t* output, uint16_t width, uint16_t height, uint32_t n_grains, uint16_t max_grains=3) {
        // Create a 2D array to store the sandpile
        std::signal(SIGINT, signal_handler);
        std::vector<std::vector<uint32_t>> sandpile(height, std::vector<uint32_t>(width, 0));

        // Add grains to the center of the sandpile
        sandpile[height / 2][width / 2] = n_grains;

        bool unstable = true;
        while (unstable) {
            unstable = false;

            // Process each cell in the sandpile
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (sandpile[y][x] > max_grains) {
                        // Distribute grains to neighboring cells
                        if (y > 0) sandpile[y-1][x] += sandpile[y][x] / 4;
                        if (y < height-1) sandpile[y+1][x] += sandpile[y][x] / 4;
                        if (x > 0) sandpile[y][x-1] += sandpile[y][x] / 4;
                        if (x < width-1) sandpile[y][x+1] += sandpile[y][x] / 4;

                        // Remove grains from current cell
                        sandpile[y][x] %= 4;
                        unstable = true;
                    }
                    output[y * width + x] = static_cast<uint8_t>(sandpile[y][x]);
                }
            }
        }
    }






    void juliaset(uint16_t* output, uint16_t width, uint16_t height, uint16_t max_iter, double c_real, double c_imag, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, bool lake=false) {
        double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        
        std::signal(SIGINT, signal_handler);
        #pragma omp parallel for schedule(dynamic)
        for (int x = 0; x < width; ++x) {
            double r_part = 0; 
            r_part = xmin + x * dx;

            for (int y = 0; y < height; ++y) {
                double i_part = ymin + y * dy;

                double z_real = 0;
                double z_imag = 0;
                z_real = r_part;
                z_imag = i_part;
                uint16_t iteration = 0;


                while (z_real * z_real + z_imag * z_imag < 4 && iteration < max_iter) {
                    double z2_real = z_real * z_real - z_imag * z_imag + c_real;
                    double z2_imag = 2 * z_real * z_imag + c_imag;
                    double temp_real = (z2_real);
                    double temp_imag = (z2_imag);

                    z_real = temp_real;
                    z_imag = temp_imag;
                    ++iteration;
                }
                

                double temp =  (z_real * z_real + z_imag * z_imag);
                if (temp < 4 && lake) {
                    output[y *width + x] = temp < 0 ? static_cast<uint16_t>(std::round((-temp/(-temp+1))*max_iter)) : static_cast<uint16_t>(std::round((temp/(temp+1))*max_iter));
                } else {
                    output[y * width + x] = iteration;
                }
            }
        }
    }




    void process_array(uint32_t* input_array, uint8_t* output_array, uint16_t width, uint16_t height, double max_value, uint16_t batch_size, double npmax) {
        std::signal(SIGINT, signal_handler);
        // Iterate over each batch
        
        #pragma omp parallel for schedule(dynamic)
        for(int i = 0; i < width * height; i += batch_size) {
            // Iterate over each value in the batch
            for(int j = i; j < i + batch_size && j < width * height; j++) {
                // Convert the value to double and scale it
                double value = (static_cast<double>((input_array[j])) / npmax) * max_value;

                // Round to nearest integer
                uint32_t rounded_value = static_cast<uint32_t>(std::round(value));
                
                // Separate RGB channels
                output_array[j * 3] = (rounded_value >> 16) & 0xFF;
                output_array[j * 3 + 1] = (rounded_value >> 8) & 0xFF;
                output_array[j * 3 + 2] = rounded_value & 0xFF;
            }
        }
    }
}



