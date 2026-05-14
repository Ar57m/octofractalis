// fract.cpp

#include "fractal_interface.h"











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
        const double xmin, const double xmax,
        const double ymin, const double ymax,
        const double* juliaset_c,
        double escape_radius,
        const bool fast_mode, const bool juliaset,
        const bool lake,
        const int top_colors_outside,
        const int top_colors_lake,
        const double* z_initial,
        double* input_array,
        const uint32_t array_size,
        const int mode)      // 2, 4, or 8
    {
        std::signal(SIGINT, signal_handler);
        if (escape_radius == 0.0) escape_radius = 2.0;
        escape_radius *= escape_radius;

        size_t exp_size = std::strlen(exp);
        const DefaultType dx = (xmax - xmin) / width;
        const DefaultType dy = (ymax - ymin) / height;
        ManualTimer timer("engine_bench.log");


        #ifdef USE_CUDA
            // --- GPU Implementation ---
            timer.start("cuda");    
            fractal_kernel_call(output, array_top_colors_outside, array_top_colors_lake, exp, exp_size, width, height, max_iter, xmin, ymin, dx, dy, juliaset_c, escape_radius, fast_mode, juliaset, lake, top_colors_outside, top_colors_lake, z_initial, input_array, array_size);
            timer.stop();
        #else

        timer.start("cpu-eval");  
        // Dispatch based on dimension
        switch (mode) {
            case 2:
                runFractalCPU<2>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            case 4:
                runFractalCPU<4>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            case 8:
                runFractalCPU<8>(output, array_top_colors_outside, array_top_colors_lake,
                                exp, exp_size, width, height, max_iter,
                                xmin, ymin, dx, dy, juliaset_c,
                                escape_radius, fast_mode, juliaset, lake,
                                top_colors_outside, top_colors_lake,
                                z_initial, input_array, array_size);
                break;
            default:
                std::cerr << "Unsupported dimension: " << mode << std::endl;
                return;
        }
        timer.stop();

        #endif
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
}