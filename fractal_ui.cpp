#include <cstdio>
#include <cmath>
#include <thread>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <sstream>
#include <algorithm>



// ImGui
#include <SDL3/SDL.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

// Image Save
#include "lodepng.h"

// Fractal Interface
#include "fractal_interface.h"
#include "math_wrapper.h"


static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture* g_pFractalTexture = nullptr;

static int g_currentTexW = 0;
static int g_currentTexH = 0;
static bool g_isFullscreen = false;

// --- Threading & Save State ---
struct SaveTask {
    std::atomic<bool> active{ false };
    std::atomic<bool> cancel{ false };
    std::string filename;
} g_saveTask;



std::thread g_renderThread;
std::atomic<bool> g_isAsyncRendering{false};
std::atomic<bool> g_abortRender{false};
std::atomic<bool> g_renderBufferReady{false};
std::vector<uint8_t> g_asyncRgbBuf;

std::atomic<double> lastInteractionTime = 0.0;
std::atomic<double>  lastGenTime = 0.0;

struct AppState {
    DefaultType offsetX = 0.0;
    DefaultType offsetY = 0.0;
    DefaultType zoom = 1.0;
    
    // Smooth interaction tracking
    DefaultType texOffsetX = 0.0;
    DefaultType texOffsetY = 0.0;
    DefaultType texZoom = 1.0;
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
    uint32_t seedLake[8] = { 0x000000, 0x001133, 0x004455, 0xCCFFFF, 0x002244, 0x000011, 0x3388AA, 0x000000 };

    int renderResMultiplier = 2;
    bool showGUI = true;
    
    int winWidth = 1280;
    int winHeight = 720;
    int renderWidth = 1280;
    int renderHeight = 720;
    const int guiWidth = 360;

    void ResetView() {
        offsetX = 0.0; offsetY = 0.0; zoom = 1.0;
        needsRender = true;
        lastInteractionTime = ImGui::GetTime();
    }
    
    void ResetAll() {
        ResetView();
        iterations = 256; escapeRadius = 2.0f; isJulia = false; fastMode = true; showLake = true;
        strncpy(expressionBuffer, "z*z+c", sizeof(expressionBuffer));
        for(int i=0; i<mode; i++) { juliaC[i] = 0.0f; zInit[i] = 0.0f; }
        juliaC[0] = -0.8f; juliaC[1] = 0.6f;
        realtimeExpression = true;
        lastGenTime = 0.0;
    }
};

AppState state;




std::string SaveState(const AppState& state, const std::string& filename = "") {
    std::ostringstream ss;
    ss << std::setprecision(18) << std::fixed;

    ss << "{\n";
    ss << "  \"offsetX\": " << state.offsetX << ",\n";
    ss << "  \"offsetY\": " << state.offsetY << ",\n";
    ss << "  \"zoom\": " << state.zoom << ",\n";
    ss << "  \"iterations\": " << state.iterations << ",\n";
    ss << "  \"escapeRadius\": " << state.escapeRadius << ",\n";
    ss << "  \"isJulia\": " << (state.isJulia ? "true" : "false") << ",\n";
    ss << "  \"fastMode\": " << (state.fastMode ? "true" : "false") << ",\n";
    ss << "  \"showLake\": " << (state.showLake ? "true" : "false") << ",\n";
    ss << "  \"ignore_it\": " << (state.ignore_it ? "true" : "false") << ",\n";
    ss << "  \"mode\": " << state.mode << ",\n";
    ss << "  \"expression\": \"" << state.expressionBuffer << "\",\n";

    // Lambda for array serialization to keep it clean
    auto writeArray = [&](const std::string& key, auto* arr, int count, bool last = false) {
        ss << "  \"" << key << "\": [";
        for(int i = 0; i < count; ++i) ss << arr[i] << (i < count - 1 ? "," : "");
        ss << "]" << (last ? "\n" : ",\n");
    };

    writeArray("juliaC", state.juliaC, 8);
    writeArray("zInit", state.zInit, 8);
    writeArray("seedOut", state.seedOut, 8);
    writeArray("seedLake", state.seedLake, 8, true);

    ss << "}";

    std::string result = ss.str();
    if (!filename.empty()) {
        std::ofstream f(filename);
        if (f.is_open()) f << result;
    }
    return result;
}

bool LoadState(AppState& state, std::string input, bool isPath = true) {
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

        std::string key = line.substr(0, colonPos);
        std::string val = line.substr(colonPos + 1);

        auto sanitize = [](std::string& s) {
            s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
                return c == ' ' || c == ',' || c == '\"' || c == '{' || c == '}' || c == '[' || c == ']';
            }), s.end());
        };

        if (key.find("offsetX") != std::string::npos) { sanitize(val); state.offsetX = std::stold(val); }
        else if (key.find("offsetY") != std::string::npos) { sanitize(val); state.offsetY = std::stold(val); }
        else if (key.find("zoom") != std::string::npos)    { sanitize(val); state.zoom = std::stold(val); }
        else if (key.find("iterations") != std::string::npos) { sanitize(val); state.iterations = std::stoi(val); }
        else if (key.find("escapeRadius") != std::string::npos) { sanitize(val); state.escapeRadius = std::stof(val); }
        else if (key.find("isJulia") != std::string::npos)     { sanitize(val); state.isJulia = (val.find("true") != std::string::npos); }
        else if (key.find("fastMode") != std::string::npos)    { sanitize(val); state.fastMode = (val.find("true") != std::string::npos); }
        else if (key.find("showLake") != std::string::npos)    { sanitize(val); state.showLake = (val.find("true") != std::string::npos); }
        else if (key.find("ignore_it") != std::string::npos)   { sanitize(val); state.ignore_it = (val.find("true") != std::string::npos); }
        else if (key.find("mode") != std::string::npos)        { sanitize(val); state.mode = std::stoi(val); }
        else if (key.find("expression") != std::string::npos) {
            size_t first = line.find('\"', colonPos);
            size_t last = line.find_last_of('\"');
            if (first != std::string::npos && last > first) {
                std::string expr = line.substr(first + 1, last - first - 1);
                strncpy(state.expressionBuffer, expr.c_str(), sizeof(state.expressionBuffer) - 1);
            }
        }
        else if (line.find('[') != std::string::npos) {
            size_t start = line.find('[');
            size_t end = line.find(']');
            std::string arrayContent = line.substr(start + 1, end - start - 1);
            std::stringstream arraySs(arrayContent);
            std::string item;
            int i = 0;
            while (std::getline(arraySs, item, ',') && i < 8) {
                if (key.find("juliaC") != std::string::npos) state.juliaC[i] = std::stof(item);
                else if (key.find("zInit") != std::string::npos) state.zInit[i] = std::stof(item);
                else if (key.find("seedOut") != std::string::npos) state.seedOut[i] = (uint32_t)std::stoul(item);
                else if (key.find("seedLake") != std::string::npos) state.seedLake[i] = (uint32_t)std::stoul(item);
                i++;
            }
        }
    }
    state.needsRender = true;
    return true;
}
void SetupCyberpunkStyle() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.ItemSpacing = ImVec2(8, 8);
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.95f);
    colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.00f, 0.40f, 0.80f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.00f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.20f, 0.60f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.00f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);
}

void ToggleFullscreen() {
    g_isFullscreen = !g_isFullscreen;
    SDL_SetWindowFullscreen(g_window, g_isFullscreen);
}

void ResizeFractalTexture(int w, int h) {
    if (w < 1 || h < 1) return;
    if (w == g_currentTexW && h == g_currentTexH && g_pFractalTexture) return;
    
    if (g_pFractalTexture) SDL_DestroyTexture(g_pFractalTexture);
    
    // Using ABGR8888 to match the [R, G, B, A] packing order
    g_pFractalTexture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    g_currentTexW = w; 
    g_currentTexH = h;
}

bool save_png_with_json(const std::string& filename, const std::vector<unsigned char>& rgba, unsigned w, unsigned h, const std::string& json) {
    lodepng::State state;
    
    // Add the JSON string to a custom "comment" or "Parameters" chunk
    // Use "Parameters" as the key to match your previous successful test
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

std::string load_json_from_png(const std::string& filename) {
    lodepng::State state;
    std::vector<unsigned char> buffer;
    std::vector<unsigned char> dummy_pixels; // We won't actually fill this
    unsigned w, h;

    unsigned error = lodepng::load_file(buffer, filename);
    if (error) return "";

    // IMPORTANT: Tell LodePNG to decode metadata but NOT the pixels
    // This makes it extremely fast.
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



void SavePNGThread(std::vector<int> pO, std::vector<int> pL, int w, int h, AppState s) {
    // PAD the RGB buffer by 8192 extra pixels to safely absorb any CUDA block edge overruns
    std::vector<uint8_t> rgb((size_t)(w * h + 8192) * 3);
    std::vector<uint8_t> rgba((size_t)w * h * 4); // No padding needed here, loop is bound to w*h
    
    DefaultType sc = 4.0 / (h * s.zoom);
    DefaultType xm = s.offsetX - (w * sc) / 2.0;
    DefaultType ym = s.offsetY - (h * sc) / 2.0;

    double cv[8]={0}, zv[8]={0}, d[1]={0};
    const int mode = state.mode;
    for (int i = 0; i < mode; i++) {
        cv[i]=(double)s.juliaC[i]; 
        zv[i]=(double)s.zInit[i]; 
    }

    fractal(rgb.data(), pO.data(), pL.data(), s.expressionBuffer,
        (uint16_t)w, (uint16_t)h, (uint16_t)s.iterations, xm, ym, sc, sc, cv, s.escapeRadius, 
        s.fastMode, s.isJulia, s.showLake, s.ignore_it, (int)pO.size(), (int)pL.size(), zv, d, 1, mode);

    if (g_saveTask.cancel) { 
        g_saveTask.active = false; 
        return; 
    }

    // Loop strictly up to w*h, completely ignoring the padded memory zone
    for(int i=0; i<w*h; i++) {
        rgba[i*4+0]=rgb[i*3+0]; 
        rgba[i*4+1]=rgb[i*3+1]; 
        rgba[i*4+2]=rgb[i*3+2]; 
        rgba[i*4+3]=255;
    }

    char tmp_fn[128];

    // get current time
    auto now = std::chrono::system_clock::now();

    // convert to time_t for calendar breakdown
    auto tt = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    #ifdef _WIN32
        localtime_s(&tm, &tt);
    #else
        localtime_r(&tt, &tm);
    #endif

    // optional milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::snprintf(tmp_fn, sizeof(tmp_fn),
        "./images/fractal_%04d%02d%02d_%02d%02d%02d.png",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    );

    std::string myJson = SaveState(s);

    save_png_with_json(tmp_fn, rgba, w, h, myJson);
    printf("Loaded JSON: %s\n", load_json_from_png(tmp_fn).c_str());
    
    g_saveTask.filename = std::string(tmp_fn);
    g_saveTask.active = false;
}

void AsyncRenderThread(std::vector<int> pO, std::vector<int> pL, int w, int h, AppState s) {
    g_isAsyncRendering = true;

    // Resize the global async buffer to match requirements
    size_t requiredSize = (size_t)(w * h + 8192) * 3;
    if (g_asyncRgbBuf.size() != requiredSize) g_asyncRgbBuf.resize(requiredSize);

    // Setup math constants (copied from your main logic)
    DefaultType sc = 4.0 / (h * s.zoom);
    DefaultType xm = s.offsetX - (w * sc) / 2.0;
    DefaultType ym = s.offsetY - (h * sc) / 2.0;
    double cv[8] = { 0 }, zv[8] = { 0 }, d[1] = { 0 };
    const int mode = state.mode;
    for (int i = 0; i < mode; i++) {
        cv[i] = (double)s.juliaC[i];
        zv[i] = (double)s.zInit[i];
    }

    uint64_t startTicks = SDL_GetTicksNS();
    // Heavy Math happens here (Main thread is now free to keep the UI alive!)
    fractal(g_asyncRgbBuf.data(), pO.data(), pL.data(), s.expressionBuffer,
        (uint16_t)w, (uint16_t)h, (uint16_t)s.iterations, xm, ym, sc, sc, cv,
        s.escapeRadius, s.fastMode, s.isJulia, s.showLake, s.ignore_it,
        (int)pO.size(), (int)pL.size(), zv, d, 1, mode);
    lastGenTime = (double)(SDL_GetTicksNS() - startTicks) / 1000000.0;
    state.texOffsetX = s.offsetX;
    state.texOffsetY = s.offsetY;
    state.texZoom = s.zoom;
    state.texW = w;
    state.texH = h;
    g_renderBufferReady = true;
    g_isAsyncRendering = false;
}

void CleanupAndExit() {
    // 1. Tell the system we are trying to stop
    g_abortRender = true; 

    // 2. Wait for the thread to finish its current job
    if (g_renderThread.joinable()) {
        g_renderThread.join(); 
    }

    // 3. Now destroy everything in order
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (g_pFractalTexture) SDL_DestroyTexture(g_pFractalTexture);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

volatile std::sig_atomic_t g_SignalExitRequested = 0;

void SignalHandler(int signum) {
    g_SignalExitRequested = 1; // Just set the flag and get out!
}



int main(int, char**) {
    std::signal(SIGINT, SignalHandler);  // Catch Ctrl+C
    std::signal(SIGTERM, SignalHandler); // Catch termination
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    g_window = SDL_CreateWindow("FRACTAL ENGINE // CYBER", state.winWidth, state.winHeight, SDL_WINDOW_RESIZABLE);
    if (!g_window) return -1;

    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);
    SetupCyberpunkStyle();

    std::string img_folder = "./images";

    try {
        if (!std::filesystem::exists(img_folder)) {
            // create_directories creates the full path including parents if needed
            std::filesystem::create_directories(img_folder);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        // Handle error (e.g., return or notify UI)
    }

    std::vector<uint8_t> rgbBuf, rgbaBuf;
    std::vector<int> palOut, palLake;

    auto regenPalettes = [&]() {
        // 1. Force the counts to be multiples of 8
        state.outColCount = (state.outColCount / 8) * 8;
        state.lakeColCount = (state.lakeColCount / 8) * 8;

        if (state.outColCount < 8) state.outColCount = 8;
        if (state.lakeColCount < 8) state.lakeColCount = 8;

        // Use reserve to prevent frequent reallocations
        if (palOut.capacity() < (size_t)state.outColCount) palOut.reserve(16384);
        if (palLake.capacity() < (size_t)state.lakeColCount) palLake.reserve(16384);

        palOut.resize(state.outColCount); 
        palLake.resize(state.lakeColCount);

        // 2. Call CPU version instead of GPU
        generate_on_cpu(state.seedOut, state.seedOutCount, state.outColCount, (uint32_t*)palOut.data());
        generate_on_cpu(state.seedLake, state.seedLakeCount, state.lakeColCount, (uint32_t*)palLake.data());
        
        if (state.realtimeExpression) state.needsRender = true;
    };
    regenPalettes();

    const int TARGET_FPS = 60;
    const int FRAME_TARGET_MS = 1000 / TARGET_FPS;

    bool done = false;
    while (!done) {
        if (g_SignalExitRequested) done = true;
        uint64_t frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(g_window)) done = true;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) { state.showGUI = !state.showGUI; if (state.realtimeExpression) state.needsRender = true; }
        if (ImGui::IsKeyPressed(ImGuiKey_F11)) ToggleFullscreen();
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)||ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) state.needsRender = true;

        if ((ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)||ImGui::IsKeyPressed(ImGuiKey_RightCtrl))&&ImGui::IsKeyPressed(ImGuiKey_S)) {
            g_saveTask.active = true;
            g_saveTask.cancel = false;
            std::thread(SavePNGThread, palOut, palLake, 
                        state.renderWidth * state.renderResMultiplier, 
                        state.renderHeight * state.renderResMultiplier, state).detach();
        }
        // 1. Fetch exact window size directly from OS every frame
        SDL_GetWindowSize(g_window, &state.winWidth, &state.winHeight);

        // 2. Prevent negative window dimensions from destroying memory allocations
        state.renderWidth = state.showGUI ? (state.winWidth - state.guiWidth) : state.winWidth;
        if (state.renderWidth < 1) state.renderWidth = 1;
        
        state.renderHeight = state.winHeight;
        if (state.renderHeight < 1) state.renderHeight = 1;
        
        // 3. Layout Calculation with CUDA Edge Padding
        ResizeFractalTexture(state.renderWidth, state.renderHeight);
        
        size_t pixelCount = (size_t)state.renderWidth * state.renderHeight;
        size_t requiredRgb = (pixelCount + 8192) * 3; // Pad by 8192 extra pixels
        size_t requiredRgba = (pixelCount + 8192) * 4;
        
        if (rgbBuf.size() != requiredRgb) {
            rgbBuf.resize(requiredRgb);
            rgbaBuf.resize(requiredRgba);
            if (state.realtimeExpression) state.needsRender = true;
        }

        bool mouseInRender = io.MousePos.x >= 0 && io.MousePos.x < state.renderWidth && io.MousePos.y >= 0 && io.MousePos.y < state.renderHeight;
        bool isFocused = (SDL_GetWindowFlags(g_window) & SDL_WINDOW_INPUT_FOCUS);
        bool interacting = false;

        if (mouseInRender && !io.WantCaptureMouse && isFocused && !g_saveTask.active) {
            if (ImGui::IsMouseDown(0)) {
                DefaultType s = 4.0 / (state.renderHeight * state.zoom);
                state.offsetX -= io.MouseDelta.x * s; 
                state.offsetY -= io.MouseDelta.y * s;
                lastInteractionTime = ImGui::GetTime(); 
                interacting = true; 
                if (state.realtimeExpression) state.needsRender = true;
            }
            if (io.MouseWheel != 0.0f) {
                DefaultType s = 4.0 / (state.renderHeight * state.zoom);
                DefaultType mx = state.offsetX - (state.renderWidth * s) / 2.0 + io.MousePos.x * s;
                DefaultType my = state.offsetY - (state.renderHeight * s) / 2.0 + io.MousePos.y * s;
                state.zoom *= (io.MouseWheel > 0 ? 1.15 : 0.87);
                if (state.zoom < 1e-15) state.zoom = 1e-15; 
                DefaultType ns = 4.0 / (state.renderHeight * state.zoom);
                state.offsetX = mx - io.MousePos.x * ns + (state.renderWidth * ns) / 2.0;
                state.offsetY = my - io.MousePos.y * ns + (state.renderHeight * ns) / 2.0;
                lastInteractionTime = ImGui::GetTime(); 
                interacting = true; 
                if (state.realtimeExpression) state.needsRender = true;
            }
        }

        if (g_renderBufferReady) {
            size_t pixelCount = (size_t)state.texW * state.texH;
            if (rgbaBuf.size() < pixelCount * 4) rgbaBuf.resize(pixelCount * 4);
            
            // Copy background RGB to foreground RGBA
            for (size_t i = 0; i < pixelCount; i++) {
                rgbaBuf[i * 4 + 0] = g_asyncRgbBuf[i * 3 + 0];
                rgbaBuf[i * 4 + 1] = g_asyncRgbBuf[i * 3 + 1];
                rgbaBuf[i * 4 + 2] = g_asyncRgbBuf[i * 3 + 2];
                rgbaBuf[i * 4 + 3] = 255;
            }

            SDL_UpdateTexture(g_pFractalTexture, nullptr, rgbaBuf.data(), state.texW * 4);
            
            g_renderBufferReady = false; 
            state.needsRender = false;
        }

        // 2. Trigger a new background render if needed and thread is idle
        bool debounceTimeout = (ImGui::GetTime() - lastInteractionTime > 0.25);
        if (state.needsRender && !interacting && debounceTimeout && !g_isAsyncRendering && !g_saveTask.active) {
            // Clean up the previous thread handle if it finished
            if (g_renderThread.joinable()) {
                g_renderThread.join();
            }
            
            g_abortRender = false;
            g_renderThread = std::thread(AsyncRenderThread, palOut, palLake, state.renderWidth, state.renderHeight, state);
        }

        if (g_pFractalTexture) {
            // 1. Current camera pixel size
            DefaultType cs = 4.0 / (state.renderHeight * state.zoom);
            // 2. Current camera top-left in world units
            DefaultType cx = state.offsetX - (state.renderWidth * cs) / 2.0;
            DefaultType cy = state.offsetY - (state.renderHeight * cs) / 2.0;

            // 3. The texture's pixel size (when it was rendered)
            DefaultType ts = 4.0 / (state.texH * state.texZoom);
            // 4. The texture's top-left in world units
            DefaultType tx = state.texOffsetX - (state.texW * ts) / 2.0;
            DefaultType ty = state.texOffsetY - (state.texH * ts) / 2.0;

            // 5. Map the texture's world position to the current screen's pixel position
            float px0 = (float)((tx - cx) / cs);
            float py0 = (float)((ty - cy) / cs);
            float px1 = (float)((tx + state.texW * ts - cx) / cs);
            float py1 = (float)((ty + state.texH * ts - cy) / cs);

            // Draw the image transformed to the current view
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)g_pFractalTexture, 
                ImVec2(px0, py0), 
                ImVec2(px1, py1)
            );
        }

        if (mouseInRender && isFocused && !interacting) {
            DefaultType s = 4.0 / (state.renderHeight * state.zoom);
            DefaultType cx = (state.offsetX - (state.renderWidth * s)/2.0) + io.MousePos.x * s;
            DefaultType cy = -((state.offsetY - (state.renderHeight * s)/2.0) + io.MousePos.y * s);
            char xb[64], yb[64], zb[64];
            snprintf(xb, 64, "X: %.8f", (double)cx); snprintf(yb, 64, "Y: %.8fi", (double)cy);
            snprintf(zb, 64, state.zoom < 1e-3 ? "Z: %.4ex" : "Z: %.4fx", (double)state.zoom);
            ImDrawList* d = ImGui::GetForegroundDrawList();
            d->AddRectFilled(ImVec2(8, state.renderHeight-40), ImVec2(500, state.renderHeight-8), IM_COL32(5,5,10,200), 4);
            d->AddText(ImVec2(16, state.renderHeight-32), IM_COL32(0,255,200,255), xb);
            d->AddText(ImVec2(180, state.renderHeight-32), IM_COL32(0,255,200,255), yb);
            d->AddText(ImVec2(360, state.renderHeight-32), IM_COL32(0,255,200,255), zb);
        }

        if (g_saveTask.active) {
            ImGui::OpenPopup("EXPORTING");
            if (ImGui::BeginPopupModal("EXPORTING", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoTitleBar)) {
                ImGui::TextColored(ImVec4(0,1,0.8f,1), "RENDER IN PROGRESS...");
                if (ImGui::Button("CANCEL", ImVec2(-1, 40))) g_saveTask.cancel = true;
                ImGui::EndPopup();
            }
        }

        if (state.showGUI && !g_saveTask.active) {
            ImGui::SetNextWindowPos(ImVec2((float)state.renderWidth, 0));
            ImGui::SetNextWindowSize(ImVec2((float)state.guiWidth, (float)state.winHeight));
            ImGui::Begin("CONSOLE", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
            double tmp = lastGenTime;
            ImGui::TextColored(ImVec4(0.9f, 0, 0.4f, 1), "FRACTAL GENERATOR [%.2f ms]", tmp);
            
            ImGui::Separator();

            float footerHeight = 100.0f;
            ImGui::BeginChild("##scroll", ImVec2(0, -footerHeight), true);

            float bw = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;
            if (ImGui::Button("RESET VIEW", ImVec2(bw, 30))) state.ResetView();
            ImGui::SameLine();
            if (ImGui::Button("RESET ALL", ImVec2(bw, 30))) state.ResetAll();

            if (ImGui::CollapsingHeader("ENGINE", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool changed = false;
                static const char* modes[] = {
                    "Complex (2D)",
                    "Quaternion (4D)",
                    "Octonion (8D)"
                };

                int currentModeIndex = 0;
                if (state.mode == 2) currentModeIndex = 0;
                else if (state.mode == 4) currentModeIndex = 1;
                else if (state.mode == 8) currentModeIndex = 2;

                if (ImGui::Combo("Mode", &currentModeIndex, modes, IM_ARRAYSIZE(modes))) {
                    state.mode = (currentModeIndex == 0) ? 2 : (currentModeIndex == 1 ? 4 : 8);

                    for (int i = state.mode; i < 8; i++) {
                        state.juliaC[i] = 0.0f;
                        state.zInit[i] = 0.0f;
                    }

                    state.needsRender = true;
                }
                if (ImGui::InputText("Expression", state.expressionBuffer, 256)) changed = true;
                if (ImGui::SliderInt("Iterations", &state.iterations, 1, 4096))  changed = true;
                if (ImGui::DragFloat("Escape", &state.escapeRadius, 0.1f, 0.1f, 256.0f)) changed = true;
                if (ImGui::Checkbox("Julia", &state.isJulia)) changed = true;
                if (ImGui::Checkbox("Fast", &state.fastMode))  changed = true;

                // Only trigger render on param change if realtime is ON
                if (changed && state.realtimeExpression) state.needsRender = true;

                ImGui::Separator();
                ImGui::Checkbox("Real-time Update", &state.realtimeExpression);

                if (!state.realtimeExpression) {
                    ImGui::SameLine();
                    // The RUN button always triggers a render
                    if (ImGui::Button("RUN", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                        state.needsRender = true;
                    }
                }
            }

            const char* labels[] = {
                "Real", "i", "j", "k", "l", "m", "n", "o"
            };

            if (ImGui::CollapsingHeader("CONSTANTS")) {
                bool changed = false;

                // --- C ---
                ImGui::TextUnformatted("Constant (C)");
                ImGui::Separator();

                if (state.isJulia) {
                    // Julia: full C is editable
                    for (int i = 0; i < state.mode; i++) {
                        char l[32];
                        snprintf(l, sizeof(l), "C.%s", labels[i]);
                        if (ImGui::DragFloat(l, &state.juliaC[i], 0.001f))
                            changed = true;
                    }
                } else {
                    // Mandelbrot: only higher dimensions are editable
                    ImGui::TextDisabled("C.real and C.i come from pixel position");

                    for (int i = 2; i < state.mode; i++) {
                        char l[32];
                        snprintf(l, sizeof(l), "C.%s", labels[i]);
                        if (ImGui::DragFloat(l, &state.juliaC[i], 0.001f))
                            changed = true;
                    }
                }

                ImGui::Spacing();

                // --- Z ---
                ImGui::TextUnformatted("Initial Z");
                ImGui::Separator();

                if (state.isJulia) {
                    // Julia: only higher dimensions editable
                    ImGui::TextDisabled("Z.real and Z.i come from pixel position");

                    for (int i = 2; i < state.mode; i++) {
                        char l[32];
                        snprintf(l, sizeof(l), "Z.%s", labels[i]);
                        if (ImGui::DragFloat(l, &state.zInit[i], 0.001f))
                            changed = true;
                    }
                } else {
                    // Mandelbrot: full Z editable
                    for (int i = 0; i < state.mode; i++) {
                        char l[32];
                        snprintf(l, sizeof(l), "Z.%s", labels[i]);
                        if (ImGui::DragFloat(l, &state.zInit[i], 0.001f))
                            changed = true;
                    }
                }

                if (changed && state.realtimeExpression)
                    state.needsRender = true;
            }

            if (ImGui::CollapsingHeader("PALETTE", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool palette_changed = false;
                if (ImGui::Checkbox("Lake", &state.showLake)) palette_changed = true;
                if (ImGui::Checkbox("Ignore Iter", &state.ignore_it)) palette_changed = true;
                bool pc = false;
                
                // SNAP the sliders to multiples of 8 so the GPU kernel never writes out of bounds!
                if (ImGui::SliderInt("Out Colors", &state.outColCount, 16, 2048)) {
                    state.outColCount = (state.outColCount / 8) * 8; 
                    pc = true;
                }
                if (ImGui::SliderInt("Lake Colors", &state.lakeColCount, 16, 2048)) {
                    state.lakeColCount = (state.lakeColCount / 8) * 8;
                    pc = true;
                }

                ImGui::Text("Outside Seeds");
                for (int i = 0; i < 8; i++) {
                    ImGui::PushID(i);
                    float c[3] = {
                        ((state.seedOut[i] >> 16) & 0xFF) / 255.f,
                        ((state.seedOut[i] >> 8) & 0xFF) / 255.f,
                        (state.seedOut[i] & 0xFF) / 255.f
                    };
                    if (ImGui::ColorEdit3("##o", c, ImGuiColorEditFlags_NoInputs)) {
                        state.seedOut[i] = ((int)(c[0] * 255) << 16) | ((int)(c[1] * 255) << 8) | (int)(c[2] * 255);
                        pc = true;
                    }
                    if ((i + 1) % 4 != 0) ImGui::SameLine();
                    ImGui::PopID();
                }

                ImGui::Text("Lake Seeds");
                for (int i = 0; i < 8; i++) {
                    ImGui::PushID(100 + i);
                    float c[3] = {
                        ((state.seedLake[i] >> 16) & 0xFF) / 255.f,
                        ((state.seedLake[i] >> 8) & 0xFF) / 255.f,
                        (state.seedLake[i] & 0xFF) / 255.f
                    };
                    if (ImGui::ColorEdit3("##l", c, ImGuiColorEditFlags_NoInputs)) {
                        state.seedLake[i] = ((int)(c[0] * 255) << 16) | ((int)(c[1] * 255) << 8) | (int)(c[2] * 255);
                        pc = true;
                    }
                    if ((i + 1) % 4 != 0) ImGui::SameLine();
                    ImGui::PopID();
                }
                if (pc) {
                    regenPalettes();
                    palette_changed = true;
                }

                if (palette_changed && state.realtimeExpression) state.needsRender = true;
            }

            ImGui::EndChild();

            ImGui::Separator();
            float bw_footer = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;

            ImGui::SliderInt("Export Scale", &state.renderResMultiplier, 1, 8);

            if (ImGui::Button("SAVE PNG", ImVec2(bw_footer, 40))) {
                g_saveTask.active = true;
                g_saveTask.cancel = false;
                std::thread(SavePNGThread, palOut, palLake, 
                            state.renderWidth * state.renderResMultiplier, 
                            state.renderHeight * state.renderResMultiplier, state).detach();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0, 0, 1));
            if (ImGui::Button("QUIT", ImVec2(bw_footer, 40))) {
                done = true;
            }
            ImGui::PopStyleColor();

            ImGui::End();
        }

        ImGui::Render();
        SDL_SetRenderDrawColorFloat(g_renderer, 0.02f, 0.02f, 0.03f, 1.0f);
        SDL_RenderClear(g_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);

        uint64_t frameEnd = SDL_GetTicks();
        uint32_t elapsed = (uint32_t)(frameEnd - frameStart);
        if (elapsed < FRAME_TARGET_MS) {
            SDL_Delay(FRAME_TARGET_MS - elapsed);
        }
    }
    CleanupAndExit();
    return 0;
}







            