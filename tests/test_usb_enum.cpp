/**
 * @file test_usb_enum.cpp
 * @brief Test USB device enumeration
 */

#include "XtionDevice.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== Xtion USB Enumeration Test ===" << std::endl;
    std::cout << std::endl;

    std::vector<xtion::DeviceInfo> devices;
    xtion::Status status = xtion::XtionDevice::enumerateDevices(devices);

    if (status != xtion::Status::OK) {
        std::cerr << "Failed to enumerate devices: " << xtion::statusToString(status) << std::endl;
        return 1;
    }

    std::cout << "Found " << devices.size() << " Xtion device(s):" << std::endl;
    std::cout << std::endl;

    for (size_t i = 0; i < devices.size(); i++) {
        const auto& info = devices[i];
        std::cout << "Device " << i << ":" << std::endl;
        std::cout << "  Vendor ID:  0x" << std::hex << info.vendor_id << std::dec << std::endl;
        std::cout << "  Product ID: 0x" << std::hex << info.product_id << std::dec << std::endl;
        std::cout << "  Serial:     " << info.serial_number << std::endl;
        std::cout << "  USB Path:   " << info.usb_path << std::endl;
        std::cout << "  Firmware:   " << static_cast<int>(info.fw_version_major)
                  << "." << static_cast<int>(info.fw_version_minor)
                  << "." << info.fw_version_build << std::endl;
        std::cout << std::endl;
    }

    if (devices.empty()) {
        std::cout << "No Xtion devices found!" << std::endl;
        std::cout << "Please check:" << std::endl;
        std::cout << "  - Device is plugged into USB 2.0 port" << std::endl;
        std::cout << "  - Device has power (blue LED should be on)" << std::endl;
        std::cout << "  - libusb driver is installed correctly" << std::endl;
        return 1;
    }

    std::cout << "Test PASSED" << std::endl;
    return 0;
}
