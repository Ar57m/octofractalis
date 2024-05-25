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

double pi = M_PI;    //3.141592653589793;
double e = M_E;       //2.718281828459045;




extern "C" {
    void scale(const float* input_tensor, float* scaled_tensor, uint32_t input_size, float new_min, float new_max) {
        std::signal(SIGINT, signal_handler);
        float current_min = *std::min_element(input_tensor, input_tensor + input_size);
        float current_max = *std::max_element(input_tensor, input_tensor + input_size);

        if (current_min == current_max){
            current_min -= 1;
        } 

        float scale_factor = (new_max - new_min) / (current_max - current_min);
        #pragma omp parallel for schedule(dynamic, 1)
        for (uint32_t i = 0; i < input_size; ++i) {
            scaled_tensor[i] = (input_tensor[i] - current_min) * scale_factor + new_min;
        }

    }
}


extern "C" {
    void fractal(uint16_t* output, uint16_t width, uint16_t height, uint16_t max_iter, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, bool lake=false) {

        std::signal(SIGINT, signal_handler);

        double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;

        #pragma omp parallel for schedule(dynamic)
        for (uint16_t x = 0; x < width; ++x) {
            double r_part = 0;
            r_part = xmin + x * dx;
            for (uint16_t y = 0; y < height; ++y) {
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
                

                if (z_real * z_real + z_imag * z_imag < 4 && lake) {
                    // Point is in the set, use lake color
                    double lake_value = log(1 + sqrt(z_real * z_real + z_imag * z_imag) );
                    uint16_t color = (uint16_t)(lake_value * max_iter) + max_iter;
                    output[y *width + x] = color;
                } else {
                    // Point is outside the set, color depends on the number of iterations
                    output[y * width + x] = iteration;
                }
            }
        }
    }
}





extern "C" {

    void juliaset(uint16_t* output, uint16_t width, uint16_t height, uint16_t max_iter, double c_real, double c_imag, double xmin=-2, double xmax=2, double ymin=-2, double ymax=2, bool lake=false) {
        double dx = (xmax - xmin) / width, dy = (ymax - ymin) / height;
        
        std::signal(SIGINT, signal_handler);
        #pragma omp parallel for schedule(dynamic)
        for (uint16_t x = 0; x < width; ++x) {
            double r_part = 0; 
            r_part = xmin + x * dx;

            for (uint16_t y = 0; y < height; ++y) {
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
                
             //   output[y * width + x] = iteration;
                if (z_real * z_real + z_imag * z_imag < 4 && lake) {
                    // Point is in the set, use lake color
                    double lake_value = log(1 + sqrt(z_real * z_real + z_imag * z_imag) );
                    uint16_t color = (uint16_t)(lake_value * max_iter) + max_iter;
                    output[y *width + x] = color;
                } else {
                    // Point is outside the set, color depends on the number of iterations
                    output[y * width + x] = iteration;
                }
            }
        }
    }

}




extern "C" {
    void process_array(uint32_t* input_array, uint8_t* output_array, uint16_t width, uint16_t height, double max_value, uint16_t batch_size, double npmax) {
        std::signal(SIGINT, signal_handler);
        // Iterate over each batch
        #pragma omp parallel for schedule(dynamic)
        for(uint32_t i = 0; i < width * height; i += batch_size) {
            // Iterate over each value in the batch
            for(uint32_t j = i; j < i + batch_size && j < width * height; j++) {
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