/**
 * @file depth_recorder.cpp
 * @brief Record depth frames to file
 *
 * This example records depth frames to a binary file for later analysis.
 */

#include "XtionDevice.h"
#include <iostream>
#include <fstream>
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

    std::cout << "=== Xtion Depth Recorder ===" << std::endl;
    std::cout << "Press Ctrl+C to stop recording" << std::endl;
    std::cout << std::endl;

    // Get output filename
    std::string filename = "depth_recording.bin";
    if (argc > 1) {
        filename = argv[1];
    }

    std::cout << "Output file: " << filename << std::endl;
    std::cout << std::endl;

    // Open output file
    std::ofstream output(filename, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return 1;
    }

    // Write header
    struct FileHeader {
        char magic[8] = {'X', 'T', 'I', 'O', 'N', 'D', 'E', 'P'};
        uint32_t version = 1;
        uint32_t width = XTION_DEPTH_WIDTH;
        uint32_t height = XTION_DEPTH_HEIGHT;
        uint32_t frame_count = 0;  // Will be updated at the end
    } header;

    output.write(reinterpret_cast<const char*>(&header), sizeof(header));

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
    std::cout << "Recording..." << std::endl;
    std::cout << std::endl;

    // Recording loop
    int frame_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (running) {
        xtion::DepthFrame frame;
        status = device.readDepthFrame(frame, 100);

        if (status == xtion::Status::OK) {
            // Write timestamp
            output.write(reinterpret_cast<const char*>(&frame.metadata.timestamp_us), sizeof(uint64_t));

            // Write frame data
            size_t data_size = frame.metadata.width * frame.metadata.height * sizeof(uint16_t);
            output.write(reinterpret_cast<const char*>(frame.data.get()), data_size);

            frame_count++;

            // Report every 30 frames
            if (frame_count % 30 == 0) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
                std::cout << "\rRecorded " << frame_count << " frames in "
                         << elapsed.count() << " seconds" << std::flush;
            }

        } else if (status != xtion::Status::ERROR_TIMEOUT) {
            std::cerr << "\nError reading frame: " << xtion::statusToString(status) << std::endl;
            break;
        }
    }

    std::cout << std::endl;
    std::cout << std::endl;

    // Update frame count in header
    output.seekp(offsetof(FileHeader, frame_count));
    uint32_t final_count = frame_count;
    output.write(reinterpret_cast<const char*>(&final_count), sizeof(uint32_t));

    output.close();

    // Cleanup
    std::cout << "Stopping depth stream..." << std::endl;
    device.stopDepthStream();

    std::cout << "Closing device..." << std::endl;
    device.close();

    std::cout << std::endl;
    std::cout << "Total frames recorded: " << frame_count << std::endl;
    std::cout << "Output file: " << filename << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
