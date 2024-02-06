#include <Windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <Managers.h>
#include <Debugging.h>

#define BOTMODE

// Main function
int main() {

    std::cout << "Starting BS3 Memory Reader" << std::endl;

    SetConsoleTitle("BS3 Memory Reader");

    if (!ItemManager::LoadContent()) {
        std::cerr << "Failed to load content" << std::endl;
        return -1;
    }

    std::thread debugThread(Debugging::DebugLoop);
    // Main loop
    long timeMillis = 0;
    bool running = true;
    while (running) {
        long currentTimeMillis = GetTickCount();
        if (currentTimeMillis - timeMillis > 17) {
            GameState::PerformActions();
            timeMillis = currentTimeMillis;
        }
    }
    return 0;
}