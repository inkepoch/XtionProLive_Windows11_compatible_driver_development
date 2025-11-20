/**
 * @file test_depth_stream.cpp
 * @brief Test depth stream acquisition
 */

#include "XtionDevice.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Xtion Depth Stream Test ===" << std::endl;
    std::cout << std::endl;

    xtion::XtionDevice device;

    std::cout << "Opening device..." << std::endl;
    xtion::Status status = device.open(0);

    if (status != xtion::Status::OK) {
        std::cerr << "Failed to open device: " << xtion::statusToString(status) << std::endl;
        return 1;
    }

    std::cout << "Device opened successfully!" << std::endl;
    std::cout << std::endl;

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

    // Capture 100 frames
    std::cout << "Capturing 100 depth frames..." << std::endl;
    int frame_count = 0;
    int success_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < 100; i++) {
        xtion::DepthFrame frame;
        status = device.readDepthFrame(frame, 1000);

        if (status == xtion::Status::OK) {
            frame_count++;
            success_count++;

            // Print frame info every 10 frames
            if (frame_count % 10 == 0) {
                std::cout << "  Frame " << frame_count
                         << " - Size: " << frame.metadata.width << "x" << frame.metadata.height
                         << ", Timestamp: " << frame.metadata.timestamp_us << " us"
                         << std::endl;

                // Calculate and display some statistics
                uint16_t* data = frame.data.get();
                int valid_pixels = 0;
                uint32_t sum = 0;
                uint16_t min_depth = 0xFFFF;
                uint16_t max_depth = 0;

                for (int p = 0; p < frame.metadata.width * frame.metadata.height; p++) {
                    if (data[p] != xtion::XTION_DEPTH_INVALID && data[p] > 0) {
                        valid_pixels++;
                        sum += data[p];
                        if (data[p] < min_depth) min_depth = data[p];
                        if (data[p] > max_depth) max_depth = data[p];
                    }
                }

                if (valid_pixels > 0) {
                    float avg_depth = sum / static_cast<float>(valid_pixels);
                    std::cout << "    Valid pixels: " << valid_pixels
                             << ", Avg depth: " << avg_depth << " mm"
                             << ", Min: " << min_depth << " mm"
                             << ", Max: " << max_depth << " mm" << std::endl;
                }
            }
        } else if (status == xtion::Status::ERROR_TIMEOUT) {
            std::cout << "  Frame " << (frame_count + 1) << " - Timeout" << std::endl;
        } else {
            std::cerr << "  Frame " << (frame_count + 1) << " - Error: "
                     << xtion::statusToString(status) << std::endl;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << std::endl;
    std::cout << "Statistics:" << std::endl;
    std::cout << "  Total frames captured: " << success_count << " / 100" << std::endl;
    std::cout << "  Total time: " << duration.count() << " ms" << std::endl;
    if (success_count > 0) {
        float fps = success_count * 1000.0f / duration.count();
        std::cout << "  Average FPS: " << fps << std::endl;
    }
    std::cout << std::endl;

    // Stop depth stream
    std::cout << "Stopping depth stream..." << std::endl;
    device.stopDepthStream();

    // Close device
    std::cout << "Closing device..." << std::endl;
    device.close();

    std::cout << std::endl;
    if (success_count >= 50) {
        std::cout << "Test PASSED (captured at least 50% of frames)" << std::endl;
        return 0;
    } else {
        std::cout << "Test FAILED (captured less than 50% of frames)" << std::endl;
        return 1;
    }
}
