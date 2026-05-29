#include <csignal>

#include <SDL3/SDL.h>
#include "imgui/imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "core/app_core.h"
#include "core/fractal_interface.h"

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture* g_pFractalTexture = nullptr;

static int g_currentTexW = 0;
static int g_currentTexH = 0;
static bool g_isFullscreen = false;


RuntimeState g_runtime;

AppState state;



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


void AsyncRenderThread(std::vector<uint32_t> pO, std::vector<uint32_t> pL, int w, int h, AppState s) {
    g_runtime.isAsyncRendering = true;

    // Resize the global async buffer to match requirements
    size_t requiredSize = (size_t)(w * h + 8192) * 4;
    if (g_runtime.asyncRgbaBuf.size() != requiredSize)
        g_runtime.asyncRgbaBuf.resize(requiredSize);

    // Setup math constants (copied from your main logic)
    DefaultType sc = 4.0 / (h * s.zoom);
    DefaultType xm = s.offsetX - (w * sc) / 2.0;
    DefaultType ym = s.offsetY - (h * sc) / 2.0;
    double cv[8] = { 0 }, zv[8] = { 0 }, d[1] = { 0 };
    const int mode = s.mode;
    for (int i = 0; i < mode; i++) {
        cv[i] = (double)s.juliaC[i];
        zv[i] = (double)s.zInit[i];
    }

    auto start = std::chrono::steady_clock::now();
    // Heavy Math happens here (Main thread is now free to keep the UI alive!)
    fractal(g_runtime.asyncRgbaBuf.data(), pO.data(), pL.data(), s.expressionBuffer,
        (uint16_t)w, (uint16_t)h, (uint16_t)s.iterations, xm, ym, sc, sc, cv,
        s.escapeRadius, s.fastMode, s.isJulia, s.showLake, s.ignore_it, g_runtime.renderStopFlag,
        (int)pO.size(), (int)pL.size(), zv, d, 1, mode);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    bool wasStopped = g_runtime.renderStopFlag.load(std::memory_order_relaxed);

    if (!wasStopped) {
        state.texOffsetX = s.offsetX;
        state.texOffsetY = s.offsetY;
        state.texZoom = s.zoom;
        state.texW = w;
        state.texH = h;

        g_runtime.lastGenTime = elapsed;
        g_runtime.renderBufferReady = true;
    }

    g_runtime.isAsyncRendering = false;
}

void CleanupAndExit() {
    // 1. Tell the system we are trying to stop
    g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);
    g_runtime.saveStopFlag.store(true, std::memory_order_relaxed);

    // 2. Wait for the thread to finish its current job
    if (g_runtime.renderThread.joinable()) {
        g_runtime.renderThread.join(); 
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

    g_window = SDL_CreateWindow("OCTO FRACTALIS", state.winWidth, state.winHeight, SDL_WINDOW_RESIZABLE);
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
            std::filesystem::create_directories(img_folder);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        printf("Error creating directory: %s\n", e.what());
    }

    std::vector<uint8_t> rgbaBuf;
    std::vector<uint32_t> palOut, palLake;
    palOut.reserve(32*(1+256)); // 32 max colors and 256 max gradients between each color
    palLake.reserve(32*(1+256));

    auto regenPalettes = [&]() {
        if (state.outGradCount < 0) state.outGradCount = 0;
        if (state.lakeGradCount < 0) state.lakeGradCount = 0;
        uint32_t countout = state.seedOut.size()*(1+state.outGradCount);
        uint32_t countlake = state.seedLake.size()*(1+state.lakeGradCount);

        palOut.resize(countout);
        palLake.resize(countlake);

        generate_on_cpu(state.seedOut.data(), state.seedOut.size(), countout, palOut.data());
        generate_on_cpu(state.seedLake.data(), state.seedLake.size(), countlake, palLake.data());
        
    };
    regenPalettes();
    

    const int TARGET_FPS = 60;
    const int FRAME_TARGET_MS = 1000 / TARGET_FPS;
    
    static std::string g_NotificationText = "";
    static uint64_t g_NotificationExpireTime = 0;

    bool done = false;
    while (!done) {
        if (g_SignalExitRequested) done = true;
        uint64_t frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(g_window)) done = true;
            
            if (event.type == SDL_EVENT_DROP_FILE) {
                const char* path = event.drop.data;

                if (path && path[0] != '\0') {
                    std::string droppedPath = path;
                    float dropX = event.drop.x;
                    float dropY = event.drop.y;

                    bool droppedOnGUI = state.showGUI && (dropX >= state.renderWidth) && (dropY > state.winHeight * 0.4f);

                    if (droppedOnGUI) {
                        if (ApplyPaletteExtraction(state, droppedPath)) {
                            regenPalettes();
                            state.needsRender = true;

                            g_NotificationText =
                                "Extracted palette from image.";
                        } else {
                            g_NotificationText =
                                "Palette extraction skipped.";
                        }

                        g_NotificationExpireTime =
                            SDL_GetTicks() + 3500;

                    }else {
                        // FRACTAL VIEW DROP (Left Side / Full Screen)
                        g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);

                        if (g_runtime.renderThread.joinable()) {
                            g_runtime.renderThread.join();
                        }

                        if (loadInputState(state, droppedPath)) {
                            size_t lastSlash = droppedPath.find_last_of("/\\");
                            std::string filename = (lastSlash == std::string::npos)
                                ? droppedPath
                                : droppedPath.substr(lastSlash + 1);

                            g_NotificationText = "Loaded View: " + filename;
                            g_NotificationExpireTime = SDL_GetTicks() + 3500;

                            regenPalettes();
                            state.needsRender = true;
                        } else {
                            g_NotificationText = "Failed to load file!";
                            g_NotificationExpireTime = SDL_GetTicks() + 3500;
                        }
                    }
                }
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) { state.showGUI = !state.showGUI; if (state.realtimeExpression) state.needsRender = true; }
        if (ImGui::IsKeyPressed(ImGuiKey_F11)) ToggleFullscreen();
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)||ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) state.needsRender = true;

        bool ctrl = ImGui::GetIO().KeyCtrl;
        if (!g_runtime.saveTask.active && ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            g_runtime.saveTask.active = true;

            g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);
            if (g_runtime.renderThread.joinable()) {
                g_runtime.renderThread.join();
            }

            if (g_runtime.saveThread.joinable()) {
                g_runtime.saveThread.join();
            }

            g_runtime.saveStopFlag.store(false, std::memory_order_relaxed);

            g_runtime.saveThread = std::thread(
                SavePNGThread,
                palOut, palLake,
                state.renderWidth * state.renderResMultiplier,
                state.renderHeight * state.renderResMultiplier,
                state,
                std::ref(g_runtime),
                fractal
            );
        }
        SDL_GetWindowSize(g_window, &state.winWidth, &state.winHeight);

        state.renderWidth = state.showGUI ? (state.winWidth - state.guiWidth) : state.winWidth;
        if (state.renderWidth < 1) state.renderWidth = 1;
        
        state.renderHeight = state.winHeight;
        if (state.renderHeight < 1) state.renderHeight = 1;
        
        size_t pixelCount = (size_t)state.renderWidth * state.renderHeight;
        size_t requiredRgba = (pixelCount + 8192) * 4;

        if (rgbaBuf.size() != requiredRgba) {
            rgbaBuf.resize(requiredRgba);
            if (state.realtimeExpression) state.needsRender = true;
        }


        bool mouseInRender = io.MousePos.x >= 0 && io.MousePos.x < state.renderWidth && io.MousePos.y >= 0 && io.MousePos.y < state.renderHeight;
        bool isFocused = (SDL_GetWindowFlags(g_window) & SDL_WINDOW_INPUT_FOCUS);
        bool interacting = false;

        if (mouseInRender && !io.WantCaptureMouse && isFocused && !g_runtime.saveTask.active) {
            if (ImGui::IsMouseDown(0)) {
                DefaultType s = 4.0 / (state.renderHeight * state.zoom);
                state.offsetX -= io.MouseDelta.x * s; 
                state.offsetY -= io.MouseDelta.y * s;
                g_runtime.lastInteractionTime = ImGui::GetTime(); 
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
                g_runtime.lastInteractionTime = ImGui::GetTime(); 
                interacting = true; 
                if (state.realtimeExpression) state.needsRender = true;
            }
            if (interacting) {
                g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);
            }
        }

        if (g_runtime.renderBufferReady) {
            ResizeFractalTexture(state.texW, state.texH);

            SDL_UpdateTexture(
                g_pFractalTexture,
                nullptr,
                g_runtime.asyncRgbaBuf.data(),
                state.texW * 4
            );

            g_runtime.renderBufferReady = false;
            // state.needsRender = false;
        }
        // Safe cleanup of finished save thread
        if (g_runtime.saveTask.active && g_runtime.saveThread.joinable()) {
            // Only join if the save thread has actually finished its work
            if (!g_runtime.saveTask.active.load(std::memory_order_relaxed)) {
                g_runtime.saveThread.join();
            }
        }

        // 2. Trigger a new background render if needed and thread is idle
        bool debounceTimeout = (ImGui::GetTime() - g_runtime.lastInteractionTime > 0.25);
        if (state.needsRender && !interacting && debounceTimeout && !g_runtime.isAsyncRendering && !g_runtime.saveTask.active) {
            // Clean up the previous thread handle if it finished
            if (g_runtime.renderThread.joinable()) {
                g_runtime.renderStopFlag.store(true, std::memory_order_relaxed); // tell it to stop
                g_runtime.renderThread.join(); // wait until it actually stops
            }
            state.needsRender = false;
            g_runtime.renderStopFlag.store(false, std::memory_order_relaxed);

            g_runtime.renderThread = std::thread(
                AsyncRenderThread,
                palOut, palLake,
                state.renderWidth,
                state.renderHeight,
                state
            );
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

        {
            ImDrawList* d = ImGui::GetForegroundDrawList();

            char buf[64];
            double tmpp = g_runtime.lastGenTime;
            if (g_runtime.isAsyncRendering || g_runtime.saveTask.active)
                snprintf(buf, 64, "rendering...");
            else if (tmpp >= 0.0)
                snprintf(buf, 64, "%.2f ms", tmpp);
            else
                snprintf(buf, 64, "cancelled");

            ImVec2 textSize = ImGui::CalcTextSize(buf);

            float pad = 10.0f;
            ImVec2 pos = ImVec2(
                state.renderWidth - textSize.x - pad * 2,
                pad
            );

            d->AddRectFilled(
                pos,
                ImVec2(pos.x + textSize.x + pad, pos.y + textSize.y + pad),
                IM_COL32(5, 5, 10, 200),
                4
            );

            d->AddText(
                ImVec2(pos.x + pad * 0.5f, pos.y + pad * 0.5f),
                IM_COL32(255, 80, 180, 255),
                buf
            );
        }
        if (!g_NotificationText.empty() && SDL_GetTicks() < g_NotificationExpireTime) {
            ImDrawList* d = ImGui::GetForegroundDrawList();
            ImVec2 textSize = ImGui::CalcTextSize(g_NotificationText.c_str());
            
            float pad = 10.0f;
            ImVec2 pos = ImVec2(pad, pad); // Top-left placement

            // Background capsule container
            d->AddRectFilled(
                pos,
                ImVec2(pos.x + textSize.x + pad * 2.0f, pos.y + textSize.y + pad),
                IM_COL32(5, 5, 10, 220), // Deep space blue/black matching your UI
                4.0f                     // Rounded corners
            );

            // Subtle colored border highlight to catch the eye beautifully
            d->AddRect(
                pos,
                ImVec2(pos.x + textSize.x + pad * 2.0f, pos.y + textSize.y + pad),
                IM_COL32(0, 255, 200, 100), // Soft cyan glow
                4.0f,
                0,
                1.0f
            );

            // Notification text
            d->AddText(
                ImVec2(pos.x + pad, pos.y + pad * 0.5f),
                IM_COL32(0, 255, 200, 255), // Bright Cyan/Teal
                g_NotificationText.c_str()
            );
        } else if (!g_NotificationText.empty()) {
            // Clear string when expired so we don't evaluate CalcTextSize every frame
            g_NotificationText = "";
        }
        if (g_runtime.saveTask.active) {
            ImGui::OpenPopup("EXPORTING");

            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImVec2 center = vp->GetCenter();

            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(260, 100), ImGuiCond_Always); // <-- key fix

            if (ImGui::BeginPopupModal("EXPORTING", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::TextColored(ImVec4(0,1,0.8f,1), "RENDER IN PROGRESS...");

                ImGui::Dummy(ImVec2(0, 10)); // spacing

                if (ImGui::Button("CANCEL", ImVec2(-1, 40))) {
                    g_runtime.saveStopFlag.store(true, std::memory_order_relaxed);
                }

                ImGui::EndPopup();
            }
        }

        if (state.showGUI && !g_runtime.saveTask.active) {
            ImGui::SetNextWindowPos(ImVec2((float)state.renderWidth, 0));
            ImGui::SetNextWindowSize(ImVec2((float)state.guiWidth, (float)state.winHeight));
            ImGui::Begin("CONSOLE", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
            ImGui::TextColored(ImVec4(0.9f, 0, 0.4f, 1), "FRACTAL VIEWER");
            
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
                if (ImGui::InputInt(
                    "Iterations",
                    &state.iterations,
                    1,
                    64
                )) {
                    state.iterations = std::clamp(
                        state.iterations,
                        1,
                        4096
                    );

                    changed = true;
                }
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

            static const char* gradItems[] = {
                "None",
                "Low (4)",
                "Medium (16)",
                "High (64)",
                "Max (256)"
            };

            static const int gradValues[] = {
                0,
                4,
                16,
                64,
                256
            };

            auto gradToIndex = [](int v) {
                switch (v) {
                    case 0:   return 0;
                    case 4:   return 1;
                    case 16:  return 2;
                    case 64:  return 3;
                    case 256: return 4;
                    default:  return 2;
                }
            };

            int outIdx = gradToIndex(state.outGradCount);

            ImGui::SetNextItemWidth(120);

            if (ImGui::Combo(
                "Out Gradient",
                &outIdx,
                gradItems,
                IM_ARRAYSIZE(gradItems)
            )) {
                state.outGradCount = gradValues[outIdx];
                palette_changed = true;
            }

            int lakeIdx = gradToIndex(state.lakeGradCount);

            ImGui::SetNextItemWidth(120);

            if (ImGui::Combo(
                "Lake Gradient",
                &lakeIdx,
                gradItems,
                IM_ARRAYSIZE(gradItems)
            )) {
                state.lakeGradCount = gradValues[lakeIdx];
                palette_changed = true;
            }

            static const char* paletteSeedItems[] = {
                " Off","  2 ","  3 ","  4 ","  5 ","  6 ","  7 ","  8 ",
                "  9 ","  10","  11","  12","  13","  14","  15","  16",
                "  17","  18","  19","  20","  21","  22","  23","  24",
                "  25","  26","  27","  28","  29","  30","  31","  32"
            };

            int outSeedCombo =
                state.paletteSeedOut <= 1 ?
                0 : state.paletteSeedOut - 1;

            ImGui::SetNextItemWidth(60);
            if (ImGui::Combo(
                "Image Seeds Out",
                &outSeedCombo,
                paletteSeedItems,
                IM_ARRAYSIZE(paletteSeedItems)
            )) {
                state.paletteSeedOut =
                    outSeedCombo == 0 ?
                    0 : outSeedCombo + 1;
            }

            int lakeSeedCombo =
                state.paletteSeedLake <= 1 ?
                0 : state.paletteSeedLake - 1;

            ImGui::SetNextItemWidth(60);
            if (ImGui::Combo(
                "Image Seeds Lake",
                &lakeSeedCombo,
                paletteSeedItems,
                IM_ARRAYSIZE(paletteSeedItems)
            )) {
                state.paletteSeedLake =
                    lakeSeedCombo == 0 ?
                    0 : lakeSeedCombo + 1;
            }

            ImGui::TextDisabled(
                "Drop image here to extract palette seeds"
            );

            ImGui::Separator();

            ImGui::Text("Outside Seeds");
            ImGui::SameLine();

            if (state.seedOut.size() < 32) {
                if (ImGui::SmallButton("+##out")) {
                    uint32_t c =
                        state.seedOut.empty()
                        ? 0xFFFFFF
                        : state.seedOut.back();

                    state.seedOut.push_back(c);

                    palette_changed = true;
                }
            }

            ImGui::SameLine();

            if (state.seedOut.size() > 2) {
                if (ImGui::SmallButton("-##out")) {
                    state.seedOut.pop_back();
                    palette_changed = true;
                }
            }
            ImGui::Spacing();
            for (int i = 0; i < (int)state.seedOut.size(); i++) {
                ImGui::PushID(i);

                float c[3] = {
                    ((state.seedOut[i] >> 16) & 0xFF) / 255.f,
                    ((state.seedOut[i] >> 8) & 0xFF) / 255.f,
                    (state.seedOut[i] & 0xFF) / 255.f
                };

                if (ImGui::ColorEdit3(
                    "##o",
                    c,
                    ImGuiColorEditFlags_NoInputs
                )) {
                    state.seedOut[i] =
                        ((int)(c[0] * 255) << 16) |
                        ((int)(c[1] * 255) << 8)  |
                        (int)(c[2] * 255);

                    palette_changed = true;
                }

                if ((i + 1) % 8 != 0)
                    ImGui::SameLine();

                ImGui::PopID();
            }

            ImGui::Spacing();

            ImGui::Text("Lake Seeds");
            ImGui::SameLine();

            if (state.seedLake.size() < 32) {
                if (ImGui::SmallButton("+##lake")) {
                    uint32_t c =
                        state.seedLake.empty()
                        ? 0xFFFFFF
                        : state.seedLake.back();

                    state.seedLake.push_back(c);

                    palette_changed = true;
                }
            }

            ImGui::SameLine();

            if (state.seedLake.size() > 2) {
                if (ImGui::SmallButton("-##lake")) {
                    state.seedLake.pop_back();
                    palette_changed = true;
                }
            }
            ImGui::Spacing();
            for (int i = 0; i < (int)state.seedLake.size(); i++) {
                ImGui::PushID(100 + i);

                float c[3] = {
                    ((state.seedLake[i] >> 16) & 0xFF) / 255.f,
                    ((state.seedLake[i] >> 8) & 0xFF) / 255.f,
                    (state.seedLake[i] & 0xFF) / 255.f
                };

                if (ImGui::ColorEdit3(
                    "##l",
                    c,
                    ImGuiColorEditFlags_NoInputs
                )) {
                    state.seedLake[i] =
                        ((int)(c[0] * 255) << 16) |
                        ((int)(c[1] * 255) << 8)  |
                        (int)(c[2] * 255);

                    palette_changed = true;
                }

                if ((i + 1) % 8 != 0)
                    ImGui::SameLine();

                ImGui::PopID();
            }

            ImGui::Spacing();

            if (palette_changed && state.realtimeExpression) {
                regenPalettes();
                g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);
                state.needsRender = true;
            }
        }

            ImGui::EndChild();

            ImGui::Separator();
            float bw_footer = (ImGui::GetContentRegionAvail().x - 8.0f) * 0.5f;

            ImGui::SliderInt("Export Scale", &state.renderResMultiplier, 1, 10);

            if (ImGui::Button("SAVE PNG", ImVec2(bw_footer, 40))) {
                if (!g_runtime.saveTask.active) {
                    g_runtime.saveTask.active = true;

                    g_runtime.renderStopFlag.store(true, std::memory_order_relaxed);
                    if (g_runtime.renderThread.joinable()) {
                        g_runtime.renderThread.join();
                    }

                    if (g_runtime.saveThread.joinable()) {
                        g_runtime.saveThread.join();
                    }

                    g_runtime.saveStopFlag.store(false, std::memory_order_relaxed);

                    g_runtime.saveThread = std::thread(
                        SavePNGThread,
                        palOut, palLake,
                        state.renderWidth * state.renderResMultiplier,
                        state.renderHeight * state.renderResMultiplier,
                        state,
                        std::ref(g_runtime),
                        fractal
                    );
                }
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







