#include "XtionDevice.h"
#include <iostream>

namespace xtion {

XtionDevice::XtionDevice()
    : is_open_(false)
    , is_initialized_(false) {
}

XtionDevice::~XtionDevice() {
    close();
}

Status XtionDevice::enumerateDevices(std::vector<DeviceInfo>& devices) {
    auto usb = std::make_shared<USBCommunication>();
    Status status = usb->initialize();
    if (status != Status::OK) {
        return status;
    }

    status = usb->enumerateDevices(devices);
    usb->shutdown();

    return status;
}

Status XtionDevice::open(int device_index) {
    if (is_open_) {
        return Status::ERROR_STREAM_ALREADY_STARTED;
    }

    // Initialize USB communication
    usb_ = std::make_shared<USBCommunication>();
    Status status = usb_->initialize();
    if (status != Status::OK) {
        return status;
    }

    // Open device
    status = usb_->open(device_index);
    if (status != Status::OK) {
        usb_->shutdown();
        usb_.reset();
        return status;
    }

    is_open_ = true;

    // Initialize protocol handler
    protocol_ = std::make_shared<ProtocolHandler>(usb_);

    // Initialize device
    status = initialize();
    if (status != Status::OK) {
        close();
        return status;
    }

    // Create stream objects
    depth_stream_ = std::make_unique<DepthStream>(usb_);
    color_stream_ = std::make_unique<ColorStream>(usb_);
    ir_stream_ = std::make_unique<IRStream>(usb_);

    is_initialized_ = true;

    std::cout << "Xtion device opened successfully" << std::endl;
    std::cout << "Serial: " << usb_->getDeviceInfo().serial_number << std::endl;
    std::cout << "Firmware: " << static_cast<int>(usb_->getDeviceInfo().fw_version_major)
              << "." << static_cast<int>(usb_->getDeviceInfo().fw_version_minor)
              << "." << usb_->getDeviceInfo().fw_version_build << std::endl;

    return Status::OK;
}

void XtionDevice::close() {
    if (!is_open_) {
        return;
    }

    // Stop all streams
    stopDepthStream();
    stopColorStream();
    stopIRStream();

    // Clean up stream objects
    depth_stream_.reset();
    color_stream_.reset();
    ir_stream_.reset();

    // Clean up protocol handler
    protocol_.reset();

    // Close USB
    if (usb_) {
        usb_->close();
        usb_->shutdown();
        usb_.reset();
    }

    is_initialized_ = false;
    is_open_ = false;

    std::cout << "Xtion device closed" << std::endl;
}

bool XtionDevice::isOpen() const {
    return is_open_ && is_initialized_;
}

const DeviceInfo& XtionDevice::getDeviceInfo() const {
    if (usb_) {
        return usb_->getDeviceInfo();
    }
    static DeviceInfo empty_info = {};
    return empty_info;
}

Status XtionDevice::startDepthStream() {
    if (!is_open_ || !depth_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    // Initialize depth camera
    Status status = protocol_->initializeDepthCamera();
    if (status != Status::OK) {
        return status;
    }

    // Start protocol-level stream
    status = protocol_->startDepthStream();
    if (status != Status::OK) {
        return status;
    }

    // Start acquisition
    return depth_stream_->start();
}

void XtionDevice::stopDepthStream() {
    if (depth_stream_ && depth_stream_->isRunning()) {
        depth_stream_->stop();
        if (protocol_) {
            protocol_->stopDepthStream();
        }
    }
}

bool XtionDevice::isDepthStreamRunning() const {
    return depth_stream_ && depth_stream_->isRunning();
}

Status XtionDevice::readDepthFrame(DepthFrame& frame, int timeout_ms) {
    if (!depth_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }
    return depth_stream_->readFrame(frame, timeout_ms);
}

void XtionDevice::setDepthRange(uint16_t min_mm, uint16_t max_mm) {
    if (depth_stream_) {
        depth_stream_->setRange(min_mm, max_mm);
    }
}

Status XtionDevice::startColorStream() {
    if (!is_open_ || !color_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    // Initialize RGB camera
    Status status = protocol_->initializeRGBCamera();
    if (status != Status::OK) {
        return status;
    }

    // Start protocol-level stream
    status = protocol_->startColorStream();
    if (status != Status::OK) {
        return status;
    }

    // Start acquisition
    return color_stream_->start();
}

void XtionDevice::stopColorStream() {
    if (color_stream_ && color_stream_->isRunning()) {
        color_stream_->stop();
        if (protocol_) {
            protocol_->stopColorStream();
        }
    }
}

bool XtionDevice::isColorStreamRunning() const {
    return color_stream_ && color_stream_->isRunning();
}

Status XtionDevice::readColorFrame(ColorFrame& frame, int timeout_ms) {
    if (!color_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }
    return color_stream_->readFrame(frame, timeout_ms);
}

void XtionDevice::setColorPixelFormat(PixelFormat format) {
    if (color_stream_) {
        color_stream_->setPixelFormat(format);
    }
}

Status XtionDevice::startIRStream() {
    if (!is_open_ || !ir_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    // Initialize IR camera
    Status status = protocol_->initializeIRCamera();
    if (status != Status::OK) {
        return status;
    }

    // Start protocol-level stream
    status = protocol_->startIRStream();
    if (status != Status::OK) {
        return status;
    }

    // Start acquisition
    return ir_stream_->start();
}

void XtionDevice::stopIRStream() {
    if (ir_stream_ && ir_stream_->isRunning()) {
        ir_stream_->stop();
        if (protocol_) {
            protocol_->stopIRStream();
        }
    }
}

bool XtionDevice::isIRStreamRunning() const {
    return ir_stream_ && ir_stream_->isRunning();
}

Status XtionDevice::readIRFrame(IRFrame& frame, int timeout_ms) {
    if (!ir_stream_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }
    return ir_stream_->readFrame(frame, timeout_ms);
}

Status XtionDevice::setRegistration(bool enable) {
    if (!protocol_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }
    return protocol_->setDepthRegistration(enable);
}

void XtionDevice::setMirroring(bool enable) {
    if (depth_stream_) {
        depth_stream_->setMirroring(enable);
    }
    if (color_stream_) {
        color_stream_->setMirroring(enable);
    }
    if (ir_stream_) {
        ir_stream_->setMirroring(enable);
    }
}

Status XtionDevice::getTemperature(float& temperature) {
    if (!protocol_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }
    return protocol_->getTemperature(temperature);
}

Status XtionDevice::initialize() {
    if (!protocol_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    // Verify firmware version
    Status status = verifyFirmwareVersion();
    if (status != Status::OK) {
        std::cerr << "Warning: Firmware version check failed. Device may not work correctly." << std::endl;
        // Continue anyway - we'll try to make it work
    }

    // Reset device
    status = protocol_->resetDevice();
    if (status != Status::OK) {
        std::cerr << "Warning: Device reset failed." << std::endl;
        // Continue anyway
    }

    // Set default stream mode
    status = protocol_->setStreamMode(0x01);
    if (status != Status::OK) {
        std::cerr << "Warning: Failed to set stream mode." << std::endl;
        // Continue anyway
    }

    return Status::OK;
}

Status XtionDevice::verifyFirmwareVersion() {
    if (!protocol_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    uint8_t major, minor;
    uint16_t build;
    Status status = protocol_->getFirmwareVersion(major, minor, build);
    if (status != Status::OK) {
        return status;
    }

    std::cout << "Firmware version: " << static_cast<int>(major)
              << "." << static_cast<int>(minor)
              << "." << build << std::endl;

    // Check if firmware version is compatible
    if (major < XTION_REQUIRED_FW_MAJOR ||
        (major == XTION_REQUIRED_FW_MAJOR && minor < XTION_REQUIRED_FW_MINOR)) {
        std::cerr << "Warning: Firmware version " << static_cast<int>(major)
                  << "." << static_cast<int>(minor) << "." << build
                  << " is older than recommended version "
                  << XTION_REQUIRED_FW_MAJOR << "."
                  << XTION_REQUIRED_FW_MINOR << "."
                  << XTION_REQUIRED_FW_BUILD << std::endl;
        return Status::ERROR_FIRMWARE_VERSION;
    }

    return Status::OK;
}

} // namespace xtion
