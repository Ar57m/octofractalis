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
    printf(
        "Fractal CLI\n\n"
        "Usage:\n"
        "  ./fractal_cli [options]\n\n"
        "Options:\n"
        "  --with <file>     Load state from JSON or PNG\n"
        "  -e \"expr\"         Override expression\n"
        "  --bench N         Run N times and average\n"
        "  -h                Show this help\n"
    );
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
void printVec(const char* name, const float* v, int count) {
    printf("%s: [", name);

    for (int i = 0; i < count; ++i) {
        printf("%.6f", v[i]);

        if (i < count - 1) {
            printf(", ");
        }
    }

    printf("]\n");
}

void printColors(const char* name, const uint32_t* colors, size_t count) {
    printf("%s: [", name);

    for (size_t i = 0; i < count; ++i) {
        printf("\"0x%06X\"", colors[i] & 0xFFFFFF);

        if (i < count - 1) {
            printf(", ");
        }
    }

    printf("]\n");
}

void printConfig() {
    printf("\n=========== FRACTAL CONFIG ===========\n");

    // Core
    printf("\n[Core]\n");
    printf("Mode:              %s\n", modeToStr(state.mode));
    printf("Expression:        %s\n", state.expressionBuffer);
    printf("Iterations:        %d\n", state.iterations);
    printf("Escape Radius:     %.6f\n", state.escapeRadius);

    // Render
    printf("\n[Render]\n");
    printf("Resolution:        %d x %d\n", state.renderWidth, state.renderHeight);
    printf("Resolution Scale:  %d\n", state.renderResMultiplier);
    printf("Zoom:              %.6f\n", state.zoom);
    printf("Offset:            (%.6f, %.6f)\n", state.offsetX, state.offsetY);

    // Flags
    printf("\n[Flags]\n");
    printf("Julia Mode:        %s\n", state.isJulia ? "yes" : "no");
    printf("Show Lake:         %s\n", state.showLake ? "yes" : "no");
    printf("Ignore Iteration:  %s\n", state.ignore_it ? "yes" : "no");

    // Vectors
    printf("\n[Vectors]\n");
    printVec("Julia C", state.juliaC, 8);
    printVec("Z Init ", state.zInit, 8);

    // Palette
    printf("\n[Palette]\n");
    printf("Out Gradient Count:   %d\n", state.outGradCount);
    printf("Lake Gradient Count:  %d\n", state.lakeGradCount);

    printColors("Seed Out ", state.seedOut.data(), state.seedOut.size());
    printColors("Seed Lake", state.seedLake.data(), state.seedLake.size());

    printf("======================================\n\n");
}

// ---------- PALETTE ----------
void regenPalettes(std::vector<uint32_t>& palOut, std::vector<uint32_t>& palLake) {
    if (state.outGradCount < 0) state.outGradCount = 0;
    if (state.lakeGradCount < 0) state.lakeGradCount = 0;

    uint32_t countout = state.seedOut.size()*(1+state.outGradCount);
    uint32_t countlake = state.seedLake.size()*(1+state.lakeGradCount);

    palOut.resize(countout);
    palLake.resize(countlake);


    palOut.resize(countout);
    palLake.resize(countlake);

    generate_on_cpu(state.seedOut.data(), state.seedOut.size(), countout, palOut.data());
    generate_on_cpu(state.seedLake.data(), state.seedLake.size(), countlake, palLake.data());
}

// ---------- RUN ----------
inline double runOnce(const std::vector<uint32_t>& palOut, const std::vector<uint32_t>& palLake) {
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
inline void runBench(
    int count,
    const std::vector<uint32_t>& palOut,
    const std::vector<uint32_t>& palLake
) {
    double total = 0.0;

    for (int i = 0; i < count; ++i) {
        if (g_stop.load()) {
            break;
        }

        double t = runOnce(palOut, palLake);
        total += t;
        printf("[%d/%d] %.6f ms\n", i + 1, count, t);
    }

    if (count > 0) {
        printf("\nAverage: %.6f ms\n", total / count);
    }
}

// ---------- MAIN ----------
int main(int argc, char** argv) {
    std::signal(SIGINT, handle_sigint);

    CLIOptions opt = parseArgs(argc, argv);
    std::string img_folder = "./images";

    try {
        if (!std::filesystem::exists(img_folder)) {
            std::filesystem::create_directories(img_folder);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        printf("Error creating directory: %s\n", e.what());
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

    std::vector<uint32_t> palOut, palLake;
    palOut.reserve(32*(1+256)); // 32 max colors and 256 max gradients between each color
    palLake.reserve(32*(1+256));

    regenPalettes(palOut, palLake);

    if (opt.bench > 1) {
        runBench(opt.bench, palOut, palLake);
    }
    else {
        double t = runOnce(palOut, palLake);
        printf("Time: %.6f ms\n", t);
    }

    if (g_stop.load()) {
        printf("\nInterrupted.\n");
    }

    return 0;
}