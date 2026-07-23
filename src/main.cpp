#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <atomic>
#include <sqlite3.h>

#include "input_stats.h"
#include "storage.h"

std::atomic<bool> g_keep_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_keep_running = false;
    }
}

int main() {
    std::cout << "KIS (Keyboard Input Statistics) started." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize input capture
    if (!input_stats::initialize()) {
        std::cerr << "Error: Could not initialize input capture." << std::endl;
        return 1;
    }

    // Initialize database
    sqlite3* db = nullptr;
    if (!storage::initialize_database("kis_stats.db", &db)) {
        std::cerr << "Error: Could not initialize database." << std::endl;
        input_stats::shutdown();
        return 1;
    }

    int accumulated_keystrokes = 0;
    int total_keystrokes = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_save_time = start_time;
    auto last_display_time = start_time;

    std::cout << "KIS ready. Monitoring keyboard activity..." << std::endl;

    // Main loop
    while (g_keep_running) {
        // Small pause to avoid CPU overload
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto now = std::chrono::steady_clock::now();

        // Accumulate keystrokes
        int keystrokes = input_stats::poll_keystrokes();
        accumulated_keystrokes += keystrokes;
        total_keystrokes += keystrokes;

        // Display stats every 5 seconds
        auto display_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_display_time).count();
        if (display_elapsed >= 5) {
            auto total_elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - start_time).count();
            int ppm = total_elapsed > 0 ? total_keystrokes / total_elapsed : 0;

            // Simple activity bar (max 50 characters)
            int bar_length = std::min(ppm, 50);
            std::string bar(bar_length, '#');
            std::string spaces(50 - bar_length, '.');

            std::cout << "\r[PPM: " << ppm << "] [" << bar << spaces << "] Total: " << total_keystrokes << "   " << std::flush;
            last_display_time = now;
        }

        // Save to database every 60 seconds
        auto save_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_time).count();
        if (save_elapsed >= 60 && accumulated_keystrokes > 0) {
            if (storage::save_keystrokes(db, accumulated_keystrokes)) {
                std::cout << "\n[Saved] " << accumulated_keystrokes << " keystrokes recorded." << std::endl;
            }
            accumulated_keystrokes = 0;
            last_save_time = now;
        }
    }

    // Save remaining data on exit
    if (accumulated_keystrokes > 0) {
        storage::save_keystrokes(db, accumulated_keystrokes);
        std::cout << "\n[Final save] " << accumulated_keystrokes << " keystrokes recorded." << std::endl;
    }

    // Close resources
    storage::close_database(db);
    input_stats::shutdown();

    std::cout << "\nKIS stopped. Total keystrokes: " << total_keystrokes << std::endl;
    return 0;
}
