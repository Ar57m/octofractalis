#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <string>

struct SaveTask {
    std::atomic<bool> active{false};
    std::string filename;
};

struct RuntimeState {
    std::thread renderThread;

    std::atomic<bool> isAsyncRendering{false};
    std::atomic<bool> renderStopFlag{false};
    std::atomic<bool> saveStopFlag{false};
    std::atomic<bool> renderBufferReady{false};

    std::vector<uint8_t> asyncRgbaBuf;

    SaveTask saveTask;

    std::atomic<double> lastInteractionTime{0.0};
    std::atomic<double> lastGenTime{0.0};
};