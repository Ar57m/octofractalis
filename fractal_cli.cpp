#include <iostream>
#include <csignal>
#include <filesystem>
#include "app_core.h"
#include "fractal_interface.h"


RuntimeState g_runtime;
AppState state;

static std::atomic<bool> g_stop(false);

// ---------- SIGNAL ----------
void handle_sigint(int) {
    g_stop.store(true);
    g_runtime.saveStopFlag.store(true);
}

// ---------- UTILS ----------
bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
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

// ---------- LOAD ----------
bool loadInputState(const std::string& path) {
    if (path.empty()) return true;

    if (!std::filesystem::exists(path)) {
        std::cerr << "File not found: " << path << "\n";
        return false;
    }

    if (endsWith(path, ".png")) {
        std::string json = load_json_from_png(path);
        if (json.empty()) {
            std::cerr << "Failed to extract JSON from PNG\n";
            return false;
        }
        return LoadState(state, json, false);
    }

    return LoadState(state, path, true);
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
    state.outColCount = (state.outColCount / 8) * 8;
    state.lakeColCount = (state.lakeColCount / 8) * 8;

    if (state.outColCount < 8) state.outColCount = 8;
    if (state.lakeColCount < 8) state.lakeColCount = 8;

    if (palOut.capacity() < (size_t)state.outColCount) palOut.reserve(16384);
    if (palLake.capacity() < (size_t)state.lakeColCount) palLake.reserve(16384);

    palOut.resize(state.outColCount);
    palLake.resize(state.lakeColCount);

    generate_on_cpu(state.seedOut, state.seedOutCount, state.outColCount, (uint32_t*)palOut.data());
    generate_on_cpu(state.seedLake, state.seedLakeCount, state.lakeColCount, (uint32_t*)palLake.data());
}

// ---------- RUN ----------
double runOnce() {
    g_runtime.saveStopFlag.store(false);

    int w = state.renderWidth * state.renderResMultiplier;
    int h = state.renderHeight * state.renderResMultiplier;

    std::vector<int> palOut, palLake;
    std::vector<uint8_t> rgbaBuf;

    regenPalettes(palOut, palLake);

    

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
void runBench(int count) {
    double total = 0.0;

    for (int i = 0; i < count; ++i) {
        if (g_stop.load()) break;

        double t = runOnce();
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

    if (!loadInputState(opt.inputPath)) {
        return 1;
    }

    applyOverrides(opt);
    printConfig();

    if (opt.bench > 1) {
        runBench(opt.bench);
    } else {
        double t = runOnce();
        std::cout << "Time: " << t << " ms\n";
    }

    if (g_stop.load()) {
        std::cout << "\nInterrupted.\n";
    }

    return 0;
}