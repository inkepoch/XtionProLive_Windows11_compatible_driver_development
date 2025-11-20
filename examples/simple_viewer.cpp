/**
 * @file simple_viewer.cpp
 * @brief Simple depth data viewer
 *
 * This example demonstrates basic usage of the Xtion driver:
 * - Opening the device
 * - Starting depth stream
 * - Reading and displaying depth frames
 */

#include "XtionDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== Xtion Simple Viewer ===" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Create device
    xtion::XtionDevice device;

    // Open device
    std::cout << "Opening device..." << std::endl;
    xtion::Status status = device.open(0);
    if (status != xtion::Status::OK) {
        std::cerr << "Failed to open device: " << xtion::statusToString(status) << std::endl;
        return 1;
    }
    std::cout << "Device opened!" << std::endl;

    // Start depth stream
    std::cout << "Starting depth stream..." << std::endl;
    status = device.startDepthStream();
    if (status != xtion::Status::OK) {
        std::cerr << "Failed to start depth stream: " << xtion::statusToString(status) << std::endl;
        device.close();
        return 1;
    }
    std::cout << "Depth stream started!" << std::endl;
    std::cout << std::endl;

    // Main loop
    int frame_count = 0;
    auto last_report_time = std::chrono::steady_clock::now();
    int frames_since_report = 0;

    while (running) {
        xtion::DepthFrame frame;
        status = device.readDepthFrame(frame, 100);

        if (status == xtion::Status::OK) {
            frame_count++;
            frames_since_report++;

            // Calculate statistics
            uint16_t* data = frame.data.get();
            int valid_pixels = 0;
            uint32_t sum = 0;
            uint16_t min_depth = 0xFFFF;
            uint16_t max_depth = 0;

            for (int i = 0; i < frame.metadata.width * frame.metadata.height; i++) {
                if (data[i] != xtion::XTION_DEPTH_INVALID && data[i] > 0) {
                    valid_pixels++;
                    sum += data[i];
                    if (data[i] < min_depth) min_depth = data[i];
                    if (data[i] > max_depth) max_depth = data[i];
                }
            }

            // Report every second
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_report_time);
            if (elapsed.count() >= 1000) {
                float fps = frames_since_report * 1000.0f / elapsed.count();

                std::cout << "\rFrame " << frame_count
                         << " | FPS: " << std::fixed << std::setprecision(1) << fps
                         << " | Valid: " << valid_pixels;

                if (valid_pixels > 0) {
                    float avg_depth = sum / static_cast<float>(valid_pixels);
                    std::cout << " | Avg: " << static_cast<int>(avg_depth) << "mm"
                             << " | Range: [" << min_depth << ", " << max_depth << "]mm";
                }

                std::cout << "          " << std::flush;

                last_report_time = now;
                frames_since_report = 0;
            }

        } else if (status != xtion::Status::ERROR_TIMEOUT) {
            std::cerr << "\nError reading frame: " << xtion::statusToString(status) << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    // Cleanup
    std::cout << "Stopping depth stream..." << std::endl;
    device.stopDepthStream();

    std::cout << "Closing device..." << std::endl;
    device.close();

    std::cout << std::endl;
    std::cout << "Total frames captured: " << frame_count << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
