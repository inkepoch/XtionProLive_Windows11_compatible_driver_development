/**
 * @file all_streams.cpp
 * @brief Capture all streams simultaneously
 *
 * This example demonstrates capturing depth, color, and IR streams at the same time.
 */

#include "XtionDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <iomanip>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== Xtion All Streams Example ===" << std::endl;
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

    // Start all streams
    std::cout << "Starting depth stream..." << std::endl;
    status = device.startDepthStream();
    if (status != xtion::Status::OK) {
        std::cerr << "Failed to start depth stream: " << xtion::statusToString(status) << std::endl;
    } else {
        std::cout << "Depth stream started!" << std::endl;
    }

    std::cout << "Starting color stream..." << std::endl;
    status = device.startColorStream();
    if (status != xtion::Status::OK) {
        std::cerr << "Warning: Failed to start color stream: " << xtion::statusToString(status) << std::endl;
    } else {
        std::cout << "Color stream started!" << std::endl;
    }

    std::cout << "Starting IR stream..." << std::endl;
    status = device.startIRStream();
    if (status != xtion::Status::OK) {
        std::cerr << "Warning: Failed to start IR stream: " << xtion::statusToString(status) << std::endl;
    } else {
        std::cout << "IR stream started!" << std::endl;
    }

    std::cout << std::endl;

    // Main loop
    int depth_frames = 0;
    int color_frames = 0;
    int ir_frames = 0;
    auto last_report_time = std::chrono::steady_clock::now();

    while (running) {
        bool got_frame = false;

        // Try to read depth frame
        if (device.isDepthStreamRunning()) {
            xtion::DepthFrame depth_frame;
            status = device.readDepthFrame(depth_frame, 10);
            if (status == xtion::Status::OK) {
                depth_frames++;
                got_frame = true;
            }
        }

        // Try to read color frame
        if (device.isColorStreamRunning()) {
            xtion::ColorFrame color_frame;
            status = device.readColorFrame(color_frame, 10);
            if (status == xtion::Status::OK) {
                color_frames++;
                got_frame = true;
            }
        }

        // Try to read IR frame
        if (device.isIRStreamRunning()) {
            xtion::IRFrame ir_frame;
            status = device.readIRFrame(ir_frame, 10);
            if (status == xtion::Status::OK) {
                ir_frames++;
                got_frame = true;
            }
        }

        // Report every second
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_report_time);
        if (elapsed.count() >= 1000) {
            std::cout << "\rDepth: " << std::setw(5) << depth_frames
                     << " | Color: " << std::setw(5) << color_frames
                     << " | IR: " << std::setw(5) << ir_frames
                     << "          " << std::flush;
            last_report_time = now;
        }

        if (!got_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    // Cleanup
    std::cout << "Stopping all streams..." << std::endl;
    device.stopDepthStream();
    device.stopColorStream();
    device.stopIRStream();

    std::cout << "Closing device..." << std::endl;
    device.close();

    std::cout << std::endl;
    std::cout << "Summary:" << std::endl;
    std::cout << "  Depth frames: " << depth_frames << std::endl;
    std::cout << "  Color frames: " << color_frames << std::endl;
    std::cout << "  IR frames:    " << ir_frames << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
