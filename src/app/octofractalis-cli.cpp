#include <iostream>
#include <csignal>
#include "core/app_core.h"
#include "core/fractal_interface.h"


RuntimeState g_runtime;
AppState state;

static std::atomic<bool> g_stop(false);

// ---------- SIGNAL ----------
void handle_sigint(int) {
    g_stop.store(true);
    g_runtime.saveStopFlag.store(true);
}

// ---------- ARGS ----------
struct CLIOptions {
    std::string inputPath;
    std::string expr;
    int bench = 1;
    bool help = false;
};

CLIOptions parseArgs(int argc, char** argv) {
    CLIOptions opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--with" && i + 1 < argc) {
            opt.inputPath = argv[++i];
        } else if (arg == "-e" && i + 1 < argc) {
            opt.expr = argv[++i];
        } else if (arg == "--bench" && i + 1 < argc) {
            opt.bench = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            opt.help = true;
        }
    }

    return opt;
}

// ---------- HELP ----------
void printHelp() {
    std::cout <<
    "Fractal CLI\n\n"
    "Usage:\n"
    "  ./fractal_cli [options]\n\n"
    "Options:\n"
    "  --with <file>     Load state from JSON or PNG\n"
    "  -e \"expr\"         Override expression\n"
    "  --bench N         Run N times and average\n"
    "  -h                Show this help\n";
}

// ---------- OVERRIDE ----------
void applyOverrides(const CLIOptions& opt) {
    if (!opt.expr.empty()) {
        snprintf(state.expressionBuffer, sizeof(state.expressionBuffer), "%s", opt.expr.c_str());
    }
}

// ---------- MODE ----------
const char* modeToStr(int mode) {
    switch (mode) {
        case 2: return "Complex";
        case 4: return "Quaternion";
        case 8: return "Octonion";
        default: return "Unknown";
    }
}

// ---------- PRINT ----------
void printConfig() {
    std::cout << "\n=== FRACTAL CONFIG ===\n";
    std::cout << "Mode:           " << modeToStr(state.mode) << "\n";
    std::cout << "Expression:     " << state.expressionBuffer << "\n";
    std::cout << "Iterations:     " << state.iterations << "\n";
    std::cout << "Escape Radius:  " << state.escapeRadius << "\n";
    std::cout << "Julia:          " << (state.isJulia ? "yes" : "no") << "\n";
    std::cout << "Resolution:     "
              << state.renderWidth << " x "
              << state.renderHeight << "\n";
    std::cout << "======================\n\n";
}

// ---------- PALETTE ----------
void regenPalettes(std::vector<int>& palOut, std::vector<int>& palLake) {
    if (state.outGradCount < 0) state.outGradCount = 0;
    if (state.lakeGradCount < 0) state.lakeGradCount = 0;
    uint32_t countout = state.seedOutCount*(1+state.outGradCount);
    uint32_t countlake = state.seedLakeCount*(1+state.lakeGradCount);
    if (palOut.capacity() < (size_t)countout) palOut.reserve(3072);
    if (palLake.capacity() < (size_t)countlake) palLake.reserve(3072);


    palOut.resize(countout);
    palLake.resize(countlake);

    generate_on_cpu(state.seedOut, state.seedOutCount, countout, (uint32_t*)palOut.data());
    generate_on_cpu(state.seedLake, state.seedLakeCount, countlake, (uint32_t*)palLake.data());
}

// ---------- RUN ----------
inline double runOnce(const std::vector<int>& palOut, const std::vector<int>& palLake) {
    g_runtime.saveStopFlag.store(false);

    int w = state.renderWidth * state.renderResMultiplier;
    int h = state.renderHeight * state.renderResMultiplier;

    SavePNGThread(
        palOut,
        palLake,
        w,
        h,
        state,
        std::ref(g_runtime),
        fractal
    );


    return g_runtime.lastGenTime;
}

// ---------- BENCH ----------
inline void runBench(int count, const std::vector<int>& palOut, const std::vector<int>& palLake) {
    double total = 0.0;
    
    for (int i = 0; i < count; ++i) {
        if (g_stop.load()) break;

        double t = runOnce(palOut, palLake);
        total += t;

        std::cout << "[" << i+1 << "/" << count << "] "
                  << t << " ms\n";
    }

    if (count > 0) {
        std::cout << "\nAverage: " << (total / count) << " ms\n";
    }
}

// ---------- MAIN ----------
int main(int argc, char** argv) {
    std::signal(SIGINT, handle_sigint);

    CLIOptions opt = parseArgs(argc, argv);
    std::string img_folder = "./images";

    try {
        if (!std::filesystem::exists(img_folder)) {
            // create_directories creates the full path including parents if needed
            std::filesystem::create_directories(img_folder);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
    }
    if (opt.help) {
        printHelp();
        return 0;
    }

    if (!loadInputState(state, opt.inputPath)) {
        return 1;
    }

    applyOverrides(opt);
    printConfig();

    std::vector<int> palOut, palLake;
    regenPalettes(palOut, palLake);

    if (opt.bench > 1) {
        runBench(opt.bench, palOut, palLake);
    } else {
        double t = runOnce(palOut, palLake);
        std::cout << "Time: " << t << " ms\n";
    }

    if (g_stop.load()) {
        std::cout << "\nInterrupted.\n";
    }

    return 0;
}