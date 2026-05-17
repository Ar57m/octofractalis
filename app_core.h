#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>

// Image Save
#include "lodepng.h"
#include "runtime.h"

struct AppState {
    double offsetX = 0.0;
    double offsetY = 0.0;
    double zoom = 1.0;
    
    // Smooth interaction tracking
    double texOffsetX = 0.0;
    double texOffsetY = 0.0;
    double texZoom = 1.0;
    int texW = 1, texH = 1;

    bool needsRender = true;
    

    int iterations = 256;
    float escapeRadius = 2.0f;
    bool isJulia = false;
    bool fastMode = true;
    bool showLake = true;
    bool ignore_it = false;
    bool realtimeExpression = true;

    int mode = 2; // 2,4,8


    float juliaC[8] = {-0.8f,0.16f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    float zInit[8] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};

    char expressionBuffer[256] = "z*z+c";
    
    int outColCount = 512;
    int lakeColCount = 512;
    
    int seedOutCount = 8;
    uint32_t seedOut[8] = { 0x050505, 0x00FFFF, 0xFF00FF, 0xFFFFFF, 0x000033, 0x004488, 0x6600AA, 0x222222 };
    int seedLakeCount = 8;
    uint32_t seedLake[8] = { 0x000000, 0x711c91, 0x004455, 0xCCFFFF, 0x002244, 0x000011, 0x00657B, 0x711c91 };

    int renderResMultiplier = 1;
    bool showGUI = true;
    
    int winWidth = 1280;
    int winHeight = 720;
    int renderWidth = 1280;
    int renderHeight = 720;
    const int guiWidth = 360;

    void ResetView();
    void ResetAll();
};

inline void AppState::ResetView() {
    offsetX = 0.0;
    offsetY = 0.0;
    zoom = 1.0;
    needsRender = true;
}

inline void AppState::ResetAll() {
    ResetView();

    iterations = 256;
    escapeRadius = 2.0f;
    isJulia = false;
    fastMode = true;
    showLake = true;
    ignore_it = false;
    realtimeExpression = true;

    snprintf(expressionBuffer, sizeof(expressionBuffer), "z*z+c");

    for (int i = 0; i < 8; i++) {
        juliaC[i] = 0.0f;
        zInit[i]  = 0.0f;
    }

    juliaC[0] = -0.8f;
    juliaC[1] = 0.6f;
}














// Helper to sanitize keys for precise matching
inline std::string cleanKey(std::string k) {
    k.erase(std::remove_if(k.begin(), k.end(), [](char c) {
        return c == ' ' || c == '\t' || c == '\"' || c == '{' || c == '}';
    }), k.end());
    return k;
}


inline std::string SaveState(const AppState& state, const std::string& filename = "") {
    std::ostringstream ss;
    ss << std::setprecision(18) << std::fixed;

    ss << "{\n";
    ss << "  \"mode\": " << state.mode << ",\n";
    ss << "  \"expression\": \"" << state.expressionBuffer << "\",\n";
    ss << "  \"iterations\": " << state.iterations << ",\n";
    ss << "  \"escapeRadius\": " << state.escapeRadius << ",\n";
    ss << "  \"isJulia\": " << (state.isJulia ? "true" : "false") << ",\n";
    ss << "  \"fastMode\": " << (state.fastMode ? "true" : "false") << ",\n";
    ss << "  \"showLake\": " << (state.showLake ? "true" : "false") << ",\n";
    ss << "  \"ignore_it\": " << (state.ignore_it ? "true" : "false") << ",\n";
    ss << "  \"offsetX\": " << state.offsetX << ",\n";
    ss << "  \"offsetY\": " << state.offsetY << ",\n";
    ss << "  \"zoom\": " << state.zoom << ",\n";



    auto writeArray = [&](const std::string& key, const auto* arr, int count, bool last = false) {
        ss << "  \"" << key << "\": [";
        for(int i = 0; i < count; ++i) ss << arr[i] << (i < count - 1 ? "," : "");
        ss << "]" << (last ? "\n" : ",\n");
    };

    writeArray("juliaC", state.juliaC, 8);
    writeArray("zInit", state.zInit, 8);
    writeArray("seedOut", state.seedOut, 8);
    writeArray("seedLake", state.seedLake, 8, true); // Kept true for valid JSON output

    ss << "}";

    std::string result = ss.str();
    if (!filename.empty()) {
        std::ofstream f(filename);
        if (f.is_open()) f << result;
    }
    return result;
}

inline bool LoadState(AppState& state, std::string input, bool isPath = true) {
    std::string content;
    if (isPath) {
        std::ifstream f(input);
        if (!f.is_open()) return false;
        content.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    } else {
        content = input;
    }

    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string rawKey = line.substr(0, colonPos);
        std::string key = cleanKey(rawKey);
        std::string val = line.substr(colonPos + 1);

        auto sanitize = [](std::string& s) {
            s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
                return c == ' ' || c == ',' || c == '\"' || c == '{' || c == '}' || c == '[' || c == ']';
            }), s.end());
        };

        if (key == "offsetX")      { sanitize(val); state.offsetX = std::stold(val); }
        else if (key == "offsetY") { sanitize(val); state.offsetY = std::stold(val); }
        else if (key == "zoom")    { sanitize(val); state.zoom = std::stold(val); }
        else if (key == "iterations")   { sanitize(val); state.iterations = std::stoi(val); }
        else if (key == "escapeRadius") { sanitize(val); state.escapeRadius = std::stof(val); }
        else if (key == "isJulia")  { sanitize(val); state.isJulia = (val.find("true") != std::string::npos); }
        else if (key == "fastMode") { sanitize(val); state.fastMode = (val.find("true") != std::string::npos); }
        else if (key == "showLake") { sanitize(val); state.showLake = (val.find("true") != std::string::npos); }
        else if (key == "ignore_it"){ sanitize(val); state.ignore_it = (val.find("true") != std::string::npos); }
        else if (key == "mode")     { sanitize(val); state.mode = std::stoi(val); }
        else if (key == "expression") {
            size_t first = line.find('\"', colonPos);
            size_t last = line.find_last_of('\"');
            if (first != std::string::npos && last > first) {
                std::string expr = line.substr(first + 1, last - first - 1);
                std::memset(state.expressionBuffer, 0, sizeof(state.expressionBuffer));
                std::strncpy(state.expressionBuffer, expr.c_str(), sizeof(state.expressionBuffer) - 1);
            }
        }
        
        // Handle array fields dynamically based on clean keys
        if (line.find('[') != std::string::npos) {
            size_t start = line.find('[');
            size_t end = line.find(']');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string arrayContent = line.substr(start + 1, end - start - 1);
                std::stringstream arraySs(arrayContent);
                std::string item;
                int i = 0;
                while (std::getline(arraySs, item, ',') && i < 8) {
                    if (key == "juliaC")         state.juliaC[i] = std::stof(item);
                    else if (key == "zInit")     state.zInit[i] = std::stof(item);
                    else if (key == "seedOut")   state.seedOut[i] = (uint32_t)std::stoul(item);
                    else if (key == "seedLake")  state.seedLake[i] = (uint32_t)std::stoul(item);
                    i++;
                }
            }
        }
    }
    state.needsRender = true;
    return true;
}











inline bool save_png_with_json(const std::string& filename, const std::vector<unsigned char>& rgba, unsigned w, unsigned h, const std::string& json) {
    lodepng::State state;
    
    lodepng_add_text(&state.info_png, "Parameters", json.c_str());

    std::vector<unsigned char> buffer;
    unsigned error = lodepng::encode(buffer, rgba, w, h, state);
    
    if (!error) {
        error = lodepng::save_file(buffer, filename);
    }

    if (error) {
        // Optional: log error lodepng_error_text(error)
        return false;
    }
    return true;
}

inline std::string load_json_from_png(const std::string& filename) {
    lodepng::State state;
    std::vector<unsigned char> buffer;
    std::vector<unsigned char> dummy_pixels; // We won't actually fill this
    unsigned w, h;

    unsigned error = lodepng::load_file(buffer, filename);
    if (error) return "";

    state.decoder.color_convert = 0; 
    state.decoder.read_text_chunks = 1; // Ensure this is on

    // Decode the header and chunks. Since we use the state, it will fill text_num.
    error = lodepng::decode(dummy_pixels, w, h, state, buffer);
    if (error) return "";

    for (size_t i = 0; i < state.info_png.text_num; ++i) {
        if (std::string(state.info_png.text_keys[i]) == "Parameters") {
            return std::string(state.info_png.text_strings[i]);
        }
    }

    return "";
}





using FractalFn = void(*)(uint8_t*,
    const int*,
    const int*,
    const char*,
    const uint16_t,
    const uint16_t,
    const uint16_t,
    const double,
    const double,
    const double,
    const double,
    const double*,
    double,
    const bool,
    const bool,
    const bool,
    const bool,
    std::atomic<bool>&,
    const int,
    const int,
    const double*,
    double*,
    const uint32_t,
    const int);




inline void SavePNGThread(
    std::vector<int> pO,
    std::vector<int> pL,
    int w, int h,
    AppState s,
    RuntimeState& g_runtime_,
    FractalFn fractalFn
) {
    g_runtime_.saveStopFlag.store(false, std::memory_order_relaxed);

    // Exact size (no padding needed)
    const size_t pixelCount = (size_t)w * h;
    std::vector<uint8_t> rgba(pixelCount * 4);

    // Compute view
    double sc = 4.0 / (h * s.zoom);
    double xm = s.offsetX - (w * sc) / 2.0;
    double ym = s.offsetY - (h * sc) / 2.0;

    // Prepare parameters
    double cv[8] = {0};
    double zv[8] = {0};
    double d[1]  = {0};

    const int mode = s.mode;

    for (int i = 0; i < mode; i++) {
        cv[i] = (double)s.juliaC[i];
        zv[i] = (double)s.zInit[i];
    }

    auto start = std::chrono::steady_clock::now();

    fractalFn(
        rgba.data(),
        pO.data(),
        pL.data(),
        s.expressionBuffer,
        (uint16_t)w,
        (uint16_t)h,
        (uint16_t)s.iterations,
        xm,
        ym,
        sc,
        sc,
        cv,
        s.escapeRadius,
        s.fastMode,
        s.isJulia,
        s.showLake,
        s.ignore_it,
        g_runtime_.saveStopFlag,
        (int)pO.size(),
        (int)pL.size(),
        zv,
        d,
        1,
        mode
    );

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    if (g_runtime_.saveStopFlag.load(std::memory_order_relaxed)) {
        g_runtime_.saveTask.active = false;
        return;
    }

    g_runtime_.lastGenTime = elapsed;

    // Filename (timestamp)
    char tmp_fn[128];

    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    #ifdef _WIN32
        localtime_s(&tm, &tt);
    #else
        localtime_r(&tt, &tm);
    #endif

    std::snprintf(tmp_fn, sizeof(tmp_fn),
        "./images/fractal_%04d%02d%02d_%02d%02d%02d.png",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    );

    // Save with metadata
    std::string myJson = SaveState(s);
    save_png_with_json(tmp_fn, rgba, w, h, myJson);

    printf("Loaded JSON: %s\n", load_json_from_png(tmp_fn).c_str());

    g_runtime_.saveTask.filename = tmp_fn;
    g_runtime_.saveTask.active = false;
}
