/**
 * @file xtion_probe.cpp
 * @brief Connectivity check: talk to the PS1080 via WinUSB/libusb
 *
 * Opens the first Xtion device, sends GET_VERSION and GET_SERIAL_NUMBER
 * over vendor control transfers and prints the result. If this works,
 * the host can talk to the device without the legacy PrimeSense driver.
 */

#include "ProtocolHandler.h"
#include "USBCommunication.h"
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {

const char* chipName(uint32_t chip) {
    switch (chip) {
        case 0x00101010: return "PS1000";
        case 0x00202020: return "PS1080";
        case 0x00212020: return "PS1080A6";
        default: return "unknown";
    }
}

} // namespace

int main() {
    std::cout << "=== Xtion Probe ===" << std::endl << std::endl;

    auto usb = std::make_shared<xtion::USBCommunication>();
    xtion::Status status = usb->initialize();
    if (status != xtion::Status::OK) {
        std::cerr << "libusb init failed: " << xtion::statusToString(status) << std::endl;
        return 1;
    }

    std::cout << "[1/3] Opening device (VID 1d27 / PID 0601)..." << std::endl;
    status = usb->open(0);
    if (status != xtion::Status::OK) {
        std::cerr << "  FAILED: " << xtion::statusToString(status) << std::endl;
        std::cerr << "  Hint: bind WinUSB to the device (interface 0) with Zadig." << std::endl;
        return 1;
    }
    std::cout << "  OK (USB serial: " << usb->getDeviceInfo().serial_number << ")" << std::endl;

    xtion::ProtocolHandler protocol(usb);

    std::cout << "[2/3] GET_VERSION..." << std::endl;
    xtion::VersionInfo version{};
    status = protocol.getVersion(version);
    if (status == xtion::Status::ERROR_TIMEOUT) {
        // The first command after power-up sometimes gets no reply
        std::this_thread::sleep_for(std::chrono::seconds(2));
        status = protocol.getVersion(version);
    }
    if (status != xtion::Status::OK) {
        std::cerr << "  FAILED: " << xtion::statusToString(status) << std::endl;
        return 1;
    }
    std::cout << "  Firmware: " << static_cast<int>(version.major) << "."
              << static_cast<int>(version.minor) << "." << version.build << std::endl;
    std::cout << "  Chip:     0x" << std::hex << std::setw(8) << std::setfill('0') << version.chip
              << std::dec << std::setfill(' ') << " (" << chipName(version.chip) << ")" << std::endl;
    std::cout << "  FPGA:     " << version.fpga << std::endl;
    std::cout << "  System:   " << version.system_version << std::endl;

    std::cout << "[3/3] GET_SERIAL_NUMBER..." << std::endl;
    std::string serial;
    status = protocol.getSerialNumber(serial);
    if (status == xtion::Status::OK) {
        std::cout << "  Serial:   " << serial << std::endl;
    } else {
        // Only supported on firmware 5.4 and later
        std::cout << "  Not available: " << xtion::statusToString(status) << std::endl;
    }

    usb->close();
    usb->shutdown();

    std::cout << std::endl << "Probe PASSED: protocol communication works." << std::endl;
    return 0;
}
