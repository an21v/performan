#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <thread>

#include "performan.h"

struct SampleGame {
    void Initialize() {
        _running.store(true);
        _updateStart = std::chrono::steady_clock::now();
    }

    void Run() {
        auto t = std::thread([this]() {
            PM_THREAD();
            bool run = _running.load();
            int count = 0;

            while (run)
            {
                PM_SCOPED_FRAME();
                _updateStart = std::chrono::steady_clock::now();

                {
                    PM_SCOPED_EVENT("Sleep");
                    std::unique_lock<std::mutex> lock(_waitMutex);
                    std::condition_variable cv;

                    cv.wait_for(lock, std::chrono::milliseconds(13));
                }

                {
                    PM_SCOPED_EVENT("Stdout");
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _updateStart);
                    std::cout << "Elapsed since last update " << elapsed.count() << " ms." << std::endl;
                }
                count++;

                if (count > 15)
                {
                    run = false;
                }
                else
                {
                    run = _running.load();
                }
            }
            });

        t.join();
    }

    void Shutdown() {
        _running.store(false);
    }

    std::mutex _waitMutex;
    std::chrono::time_point<std::chrono::steady_clock> _updateStart;
    std::atomic<bool> _running{ false };
};

int main() {
    Performan::Profiler::CreateInstance();
    Performan::Profiler::GetInstance()->SetAllocator(&Performan::GetDefaultAllocator());

    SampleGame game;

    game.Initialize();
    game.Run();

    Performan::Profiler::DestroyInstance();
}