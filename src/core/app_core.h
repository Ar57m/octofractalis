#pragma once
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <cmath>

// Image Save an load
#include "ext/lodepng.h"
#include "stb_image.h"

#include "core/runtime.h"



struct AppState {
    double offsetX = 0.0;
    double offsetY = 0.0;
    double zoom = 0.8;
    
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
    
    int outGradCount = 64;
    int lakeGradCount = 64;

    int paletteSeedOut= 8;
    int paletteSeedLake= 8;

    std::vector<uint32_t> seedOut = { 0x050505, 0x00FFFF, 0xFF00FF, 0xFFFFFF, 0x000033, 0x004488, 0x6600AA, 0x222222 };

    std::vector<uint32_t> seedLake = { 0x000000, 0x711c91, 0x004455, 0xCCFFFF, 0x002244, 0x000011, 0x00657B, 0x711c91 };

    AppState() {
        seedOut.reserve(32);
        seedLake.reserve(32);
    }

    int renderResMultiplier = 1;
    bool showGUI = true;
    
    int winWidth = 1280;
    int winHeight = 720;
    int renderWidth = 1024;
    int renderHeight = 1024;
    const int guiWidth = 360;

    void ResetView();
    void ResetAll();
};

inline void AppState::ResetView() {
    offsetX = 0.0;
    offsetY = 0.0;
    zoom = 0.8;
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






struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct HSV {
    float h;
    float s;
    float v;
};

struct PaletteColor {
    RGB rgb;
    HSV hsv;
    uint32_t count;
};





// Palette extraction part

inline HSV RGBToHSV(uint8_t r, uint8_t g, uint8_t b) {

    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxv = std::max({ rf, gf, bf });
    float minv = std::min({ rf, gf, bf });

    float delta = maxv - minv;

    HSV out {};

    if (delta > 0.0f) {

        if (maxv == rf) {
            out.h = 60.0f * std::fmod(((gf - bf) / delta), 6.0f);
        }
        else if (maxv == gf) {
            out.h = 60.0f * (((bf - rf) / delta) + 2.0f);
        }
        else {
            out.h = 60.0f * (((rf - gf) / delta) + 4.0f);
        }

        if (out.h < 0.0f) {
            out.h += 360.0f;
        }
    }

    out.s = maxv <= 0.0f ? 0.0f : (delta / maxv) * 100.0f;
    out.v = maxv * 100.0f;

    return out;
}

inline uint32_t PackRGB(uint8_t r, uint8_t g, uint8_t b) {
    return
        (uint32_t(r) << 16) |
        (uint32_t(g) << 8)  |
        uint32_t(b);
}

inline bool IsSimilar(
    const PaletteColor& a,
    const PaletteColor& b,
    int rThreshold,
    int gThreshold,
    int bThreshold,
    float hThreshold,
    float sThreshold,
    float vThreshold
) {

    const int dr =
        std::abs(int(a.rgb.r) - int(b.rgb.r));

    const int dg =
        std::abs(int(a.rgb.g) - int(b.rgb.g));

    const int db =
        std::abs(int(a.rgb.b) - int(b.rgb.b));

    const float dh =
        std::abs(a.hsv.h - b.hsv.h);

    const float ds =
        std::abs(a.hsv.s - b.hsv.s);

    const float dv =
        std::abs(a.hsv.v - b.hsv.v);

    const bool rgbSimilar =
        dr <= rThreshold &&
        dg <= gThreshold &&
        db <= bThreshold;

    const bool hsvSimilar =
        dh <= hThreshold &&
        ds <= sThreshold &&
        dv <= vThreshold;

    return rgbSimilar || hsvSimilar;
}

inline bool ExtractPaletteFromImage(
    const std::string& path,
    std::vector<uint32_t>& outPalette,
    int colorCount,
    int rThreshold = 40,
    int gThreshold = 40,
    int bThreshold = 40,
    float hThreshold = 48.0f,
    float sThreshold = 30.0f,
    float vThreshold = 30.0f
) {

    if (colorCount <= 1) {
        return false;
    }

    if (colorCount > 32) {
        colorCount = 32;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data =
        stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            3
        );

    if (!data) {

        printf(
            "Failed to load image: %s\n",
            stbi_failure_reason()
        );

        return false;
    }

    std::unordered_map<uint32_t, uint32_t> frequency;

    const int pixelCount = width * height;

    for (int i = 0; i < pixelCount; ++i) {

        const uint8_t r = data[i * 3 + 0];
        const uint8_t g = data[i * 3 + 1];
        const uint8_t b = data[i * 3 + 2];

        const uint32_t packed =
            PackRGB(r, g, b);

        frequency[packed]++;
    }

    std::vector<PaletteColor> colors;
    colors.reserve(frequency.size());

    for (const auto& pair : frequency) {

        const uint32_t packed = pair.first;

        PaletteColor c {};

        c.rgb.r = (packed >> 16) & 255;
        c.rgb.g = (packed >> 8) & 255;
        c.rgb.b = packed & 255;

        c.hsv =
            RGBToHSV(
                c.rgb.r,
                c.rgb.g,
                c.rgb.b
            );

        c.count = pair.second;

        colors.push_back(c);
    }

    std::sort(
        colors.begin(),
        colors.end(),
        [](const PaletteColor& a, const PaletteColor& b) {
            return a.count > b.count;
        }
    );

    std::vector<PaletteColor> filtered;
    filtered.reserve(colorCount);

    for (const PaletteColor& c : colors) {

        bool similar = false;

        for (const PaletteColor& existing : filtered) {

            if (
                IsSimilar(
                    c,
                    existing,
                    rThreshold,
                    gThreshold,
                    bThreshold,
                    hThreshold,
                    sThreshold,
                    vThreshold
                )
            ) {
                similar = true;
                break;
            }
        }

        if (!similar) {

            filtered.push_back(c);

            if ((int)filtered.size() >= colorCount) {
                break;
            }
        }
    }

    if (filtered.size() < 2) {

        stbi_image_free(data);
        return false;
    }

    outPalette.clear();
    outPalette.resize(filtered.size());

    for (size_t i = 0; i < filtered.size(); ++i) {

        outPalette[i] =
            PackRGB(
                filtered[i].rgb.r,
                filtered[i].rgb.g,
                filtered[i].rgb.b
            );
    }

    stbi_image_free(data);

    return true;
}


inline bool ApplyPaletteExtraction(
    AppState& state,
    const std::string& path
) {
    bool changed = false;

    if (
        state.paletteSeedOut > 1 &&
        state.paletteSeedOut <= 32
    ) {

        changed |= ExtractPaletteFromImage(
            path,
            state.seedOut,
            state.paletteSeedOut
        );
    }

    if (
        state.paletteSeedLake > 1 &&
        state.paletteSeedLake <= 32
    ) {

        changed |= ExtractPaletteFromImage(
            path,
            state.seedLake,
            state.paletteSeedLake
        );
    }

    return changed;
}








// Helper to sanitize keys for precise matching
inline std::string cleanKey(std::string k) {
    k.erase(std::remove_if(k.begin(), k.end(), [](char c) {
        return c == ' ' || c == '\t' || c == '\"' || c == '{' || c == '}';
    }), k.end());
    return k;
}
inline uint32_t parseColorItem(std::string item) {
    // Aggressively strip out all JSON array garbage: [, ], ", commas, and spaces
    item.erase(std::remove_if(item.begin(), item.end(), [](unsigned char c) {
        return c == '[' || c == ']' || c == '"' || c == ',' || std::isspace(c);
    }), item.end());

    // If the item ended up completely empty after cleaning, bail early
    if (item.empty()) return 0;

    try {
        // Now item is guaranteed to be just "0x00ffff", "0x711c91", etc.
        unsigned long val = std::stoul(item, nullptr, 0);
        
        // Cap it to 24-bit color max
        return std::min<uint32_t>(val, 0xffffff); 
    } 
    catch (...) {
        return 0; // Fallback safely if data is completely corrupted
    }
}
inline constexpr bool endsWith(const char* expr, const char* token) {
    // 1. Calculate length of expr and token
    int exprLen = 0;
    while (expr[exprLen] != '\0') exprLen++;

    int tokenLen = 0;
    while (token[tokenLen] != '\0') tokenLen++;

    // 2. If token is longer than expr, it cannot be a suffix
    if (tokenLen > exprLen) return false;

    // 3. Compare from the offset
    int startPos = exprLen - tokenLen;
    for (int i = 0; i < tokenLen; ++i) {
        if (expr[startPos + i] != token[i]) {
            return false;
        }
    }

    return true;
}
inline bool endsWith(const std::string& str, const char* token) {
    return endsWith(str.c_str(), token);
}
inline void SanitizeExpression(const char* src, char dst[256]) {
    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t j = 0;

    for (size_t i = 0; src[i] && j < 255; ++i) {
        unsigned char c = (unsigned char)src[i];

        if (std::isspace(c))
            continue;

        // Allow only characters your parser understands.
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '^' || c == '%' ||
            c == '(' || c == ')' ||
            c == '[' || c == ']' ||
            c == ',' || c == '.' ||
            c == '<' || c == '>' ||
            c == '=' || c == '&')
        {
            dst[j++] = (char)c;
        }
    }

    dst[j] = '\0';
}





inline std::string SaveState(const AppState& state, const std::string& filename = "", const bool saveJson = false) {
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
    ss << "  \"renderWidth\": " << state.renderWidth << ",\n";
    ss << "  \"renderHeight\": " << state.renderHeight << ",\n";
    ss << "  \"renderResMultiplier\": " << state.renderResMultiplier << ",\n";
    ss << "  \"lakeGradCount\": " << state.lakeGradCount << ",\n";
    ss << "  \"outGradCount\": " << state.outGradCount << ",\n";
    ss << "  \"filename\": \"" << filename << "\",\n";


    auto writeArray = [&](const std::string& key, const auto* arr, int count, bool last = false, bool asHex = false) {
        ss << "  \"" << key << "\": [";
        for(int i = 0; i < count; ++i) {
            if (asHex) {
                // Limit max value to 24-bit color (0xffffff)
                uint32_t color = std::min<uint32_t>(arr[i], 0xffffff);
                
                // Format as "0x000000" padded nicely
                ss << "\"0x" << std::setfill('0') << std::setw(6) << std::hex << color << "\"";
            } else {
                ss << std::dec << arr[i]; // Ensure it's decimal for normal numbers
            }
            
            ss << (i < count - 1 ? ", " : "");
        }
        // Reset stream back to decimal just in case
        ss << std::dec << "]" << (last ? "\n" : ",\n");
    };

    writeArray("juliaC", state.juliaC, 8);
    writeArray("zInit", state.zInit, 8);
    writeArray("seedOut", state.seedOut.data(), state.seedOut.size(), false, true);
    writeArray("seedLake", state.seedLake.data(), state.seedLake.size(), true, true); // Kept true for valid JSON output

    ss << "}";

    std::string result = ss.str();
    if (!filename.empty() && saveJson) {
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
        else if (key == "renderHeight")   { sanitize(val); state.renderHeight = std::stoi(val); }
        else if (key == "renderWidth")   { sanitize(val); state.renderWidth = std::stoi(val); }
        else if (key == "renderResMultiplier")   { sanitize(val); state.renderResMultiplier = std::stoi(val); }
        else if (key == "lakeGradCount")   { sanitize(val); state.lakeGradCount = std::stoi(val); }
        else if (key == "outGradCount")   { sanitize(val); state.outGradCount = std::stoi(val); }
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

            if (start != std::string::npos &&
                end   != std::string::npos &&
                end > start
            ) {
                std::string arrayContent =
                    line.substr(start + 1, end - start - 1);

                std::stringstream arraySs(arrayContent);
                std::string item;

                if (key == "juliaC") {
                    int i = 0;
                    while (std::getline(arraySs, item, ',') && i < 8 ) {
                        state.juliaC[i] = std::stof(item);
                        i++;
                    }
                }
                else if (key == "zInit") {
                    int i = 0;
                    while (std::getline(arraySs, item, ',') && i < 8 ) {
                        state.zInit[i] = std::stof(item);
                        i++;
                    }
                }
                else if ( key == "seedOut" || key == "seedLake" ) {
                    std::vector<uint32_t>& target =
                        (key == "seedOut")
                        ? state.seedOut
                        : state.seedLake;

                    int count = 0;

                    // Count items first
                    std::stringstream countSs(arrayContent);

                    while (
                        std::getline(countSs, item, ',')
                    ) {
                        count++;
                    }
                    // Clamp between 2 and 32
                    count = std::max(2, std::min(count, 32));

                    // Resize once
                    target.resize(count);

                    // Parse again and assign directly
                    std::stringstream parseSs(arrayContent);
                    int i = 0;

                    while (std::getline(parseSs, item, ',') && i < count ) {
                        target[i] = parseColorItem(item);
                        i++;
                    }

                    // Fill missing entries if needed
                    while (i < count) {
                        target[i] =
                            (i > 0)
                            ? target[i - 1]
                            : 0x000000;

                        i++;
                    }
                    
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

inline bool loadInputState(AppState& state_, const std::string& path) {
    if (path.empty()) return true;

    if (!std::filesystem::exists(path)) {
        printf("File not found: %s\n", path.c_str());
        return false;
    }

    if (endsWith(path, ".png")) {
        std::string json = load_json_from_png(path);
        // printf("JSON included in IMG Metadata: %s\n", json.c_str());
        if (json.empty()) {
            printf("Failed to extract JSON from PNG\n");
            return false;
        }
        return LoadState(state_, json, false);
    }

    return LoadState(state_, path, true);
}


inline std::string MakeTimestampFilename(
    const std::string& folder,
    const std::string& prefix = "fractal",
    const std::string& extension = "png"
) {
    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::ostringstream ss;
    ss << prefix << '_'
       << std::setfill('0')
       << std::setw(4) << (tm.tm_year + 1900)
       << std::setw(2) << (tm.tm_mon + 1)
       << std::setw(2) << tm.tm_mday
       << '_'
       << std::setw(2) << tm.tm_hour
       << std::setw(2) << tm.tm_min
       << std::setw(2) << tm.tm_sec
       << '_'
       << std::setw(3) << ms.count()
       << '.' << extension;

    return (std::filesystem::path(folder) / ss.str()).string();
}




using FractalFn = void(*)(uint8_t*,
    const uint32_t*,
    const uint32_t*,
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
    std::vector<uint32_t> pO,
    std::vector<uint32_t> pL,
    int w, int h,
    AppState s,
    RuntimeState& g_runtime_,
    FractalFn fractalFn,
    const std::string& path_ // full path or relative
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

    std::string filePath = MakeTimestampFilename(path_);


    std::string myJson = SaveState(s, filePath);
    save_png_with_json(filePath, rgba, w, h, myJson);

    // printf("JSON included in IMG Metadata: %s\n", load_json_from_png(tmp_fn).c_str());

    g_runtime_.saveTask.filename = filePath;
    g_runtime_.saveTask.active = false;
}










// Portable nearest-even rounding
inline int round_div_nearest_even(long long numer, int denom) {
    long long q = numer / denom;
    long long rem = numer % denom;
    long long twice = rem * 2;
    if (twice < denom) return (int)q;
    if (twice > denom) return (int)(q + 1);
    return (int)((q % 2 == 0) ? q : q + 1);
}

// CPU version of the RGB Lerp
inline uint32_t rgbLerp_cpu(uint32_t c0, uint32_t c1, int step, int steps) {
    int r0 = (c0 >> 16) & 0xFF;
    int g0 = (c0 >> 8) & 0xFF;
    int b0 = c0 & 0xFF;
    int r1 = (c1 >> 16) & 0xFF;
    int g1 = (c1 >> 8) & 0xFF;
    int b1 = c1 & 0xFF;
    
    int denom = steps + 1;
    int r = round_div_nearest_even((long long)r0 * (denom - step) + (long long)r1 * step, denom);
    int g = round_div_nearest_even((long long)g0 * (denom - step) + (long long)g1 * step, denom);
    int b = round_div_nearest_even((long long)b0 * (denom - step) + (long long)b1 * step, denom);
    
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

inline void generate_on_cpu(const uint32_t* h_colors, int numColors, int totalOutputSize, uint32_t* h_out) {
    if (totalOutputSize <= 0 || numColors <= 0) return;

    int outSizePerColor = totalOutputSize / numColors;
    int S = outSizePerColor - 1;

    for (int i = 0; i < totalOutputSize; ++i) {
        int colorIndex = i / outSizePerColor;
        int pos = i % outSizePerColor;

        if (colorIndex >= numColors) break;

        uint32_t c0 = h_colors[colorIndex];
        if (pos == 0 || S <= 0) {
            h_out[i] = c0;
        } else {
            uint32_t c1 = h_colors[(colorIndex + 1) % numColors];
            h_out[i] = rgbLerp_cpu(c0, c1, pos, S);
        }
    }
}