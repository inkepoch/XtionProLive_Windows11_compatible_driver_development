/**
 * @file test_device_open.cpp
 * @brief Test device open and basic operations
 */

#include "XtionDevice.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Xtion Device Open Test ===" << std::endl;
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

    // Get device info
    const auto& info = device.getDeviceInfo();
    std::cout << "Device Information:" << std::endl;
    std::cout << "  Serial:   " << info.serial_number << std::endl;
    std::cout << "  Firmware: " << static_cast<int>(info.fw_version_major)
              << "." << static_cast<int>(info.fw_version_minor)
              << "." << info.fw_version_build << std::endl;
    std::cout << std::endl;

    // Test temperature reading
    std::cout << "Reading device temperature..." << std::endl;
    float temperature;
    status = device.getTemperature(temperature);
    if (status == xtion::Status::OK) {
        std::cout << "  Temperature: " << temperature << "°C" << std::endl;
    } else {
        std::cout << "  Temperature reading not available" << std::endl;
    }
    std::cout << std::endl;

    // Keep device open for a moment
    std::cout << "Keeping device open for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Closing device..." << std::endl;
    device.close();

    std::cout << std::endl;
    std::cout << "Test PASSED" << std::endl;
    return 0;
}
