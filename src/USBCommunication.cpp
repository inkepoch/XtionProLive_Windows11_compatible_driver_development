#include "USBCommunication.h"
#include <cstring>
#include <iostream>

namespace xtion {

USBCommunication::USBCommunication()
    : context_(nullptr)
    , device_handle_(nullptr)
    , interface_claimed_(false) {
    std::memset(&device_info_, 0, sizeof(device_info_));
}

USBCommunication::~USBCommunication() {
    close();
    shutdown();
}

Status USBCommunication::initialize() {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    int result = libusb_init(&context_);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to initialize libusb: " << libusb_error_name(result) << std::endl;
        return Status::ERROR_USB_OPEN_FAILED;
    }

    // Set log level for debugging
    #ifdef DEBUG
    libusb_set_option(context_, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
    #endif

    return Status::OK;
}

void USBCommunication::shutdown() {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (context_) {
        libusb_exit(context_);
        context_ = nullptr;
    }
}

Status USBCommunication::enumerateDevices(std::vector<DeviceInfo>& devices) {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (!context_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    libusb_device** device_list;
    ssize_t count = libusb_get_device_list(context_, &device_list);
    if (count < 0) {
        return Status::ERROR_USB_NOT_FOUND;
    }

    devices.clear();

    for (ssize_t i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device_list[i], &desc) == LIBUSB_SUCCESS) {
            // Check if this is an Xtion device
            if (desc.idVendor == XTION_VENDOR_ID &&
                (desc.idProduct == XTION_PRODUCT_ID || desc.idProduct == XTION_PRODUCT_ID_BOOT)) {

                DeviceInfo info;
                info.vendor_id = desc.idVendor;
                info.product_id = desc.idProduct;

                // Get serial number
                libusb_device_handle* handle;
                if (libusb_open(device_list[i], &handle) == LIBUSB_SUCCESS) {
                    char serial[256];
                    if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber,
                                                          (unsigned char*)serial, sizeof(serial)) > 0) {
                        info.serial_number = serial;
                    }
                    libusb_close(handle);
                }

                // Get USB path
                uint8_t bus = libusb_get_bus_number(device_list[i]);
                uint8_t addr = libusb_get_device_address(device_list[i]);
                info.usb_path = std::to_string(bus) + "-" + std::to_string(addr);

                devices.push_back(info);
            }
        }
    }

    libusb_free_device_list(device_list, 1);

    return devices.empty() ? Status::ERROR_USB_NOT_FOUND : Status::OK;
}

Status USBCommunication::open(int device_index) {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (!context_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    if (device_handle_) {
        return Status::ERROR_STREAM_ALREADY_STARTED;
    }

    // Find the device
    libusb_device** device_list;
    ssize_t count = libusb_get_device_list(context_, &device_list);
    if (count < 0) {
        return Status::ERROR_USB_NOT_FOUND;
    }

    int found_index = 0;
    libusb_device* target_device = nullptr;

    for (ssize_t i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device_list[i], &desc) == LIBUSB_SUCCESS) {
            if (desc.idVendor == XTION_VENDOR_ID && desc.idProduct == XTION_PRODUCT_ID) {
                if (found_index == device_index) {
                    target_device = device_list[i];
                    break;
                }
                found_index++;
            }
        }
    }

    if (!target_device) {
        libusb_free_device_list(device_list, 1);
        return Status::ERROR_USB_NOT_FOUND;
    }

    // Open the device
    int result = libusb_open(target_device, &device_handle_);
    libusb_free_device_list(device_list, 1);

    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to open device: " << libusb_error_name(result) << std::endl;
        return Status::ERROR_USB_OPEN_FAILED;
    }

    // Get device info
    libusb_device_descriptor desc;
    libusb_get_device_descriptor(libusb_get_device(device_handle_), &desc);
    device_info_.vendor_id = desc.idVendor;
    device_info_.product_id = desc.idProduct;

    // Read serial number
    std::string serial;
    readSerialNumber(serial);
    device_info_.serial_number = serial;

    // Claim interface
    Status status = claimInterface();
    if (status != Status::OK) {
        libusb_close(device_handle_);
        device_handle_ = nullptr;
        return status;
    }

    // Read firmware version
    readFirmwareVersion(device_info_.fw_version_major,
                       device_info_.fw_version_minor,
                       device_info_.fw_version_build);

    return Status::OK;
}

void USBCommunication::close() {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (device_handle_) {
        releaseInterface();
        libusb_close(device_handle_);
        device_handle_ = nullptr;
    }
}

Status USBCommunication::claimInterface() {
    if (!device_handle_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    // Detach kernel driver if active
    if (libusb_kernel_driver_active(device_handle_, INTERFACE_NUMBER) == 1) {
        libusb_detach_kernel_driver(device_handle_, INTERFACE_NUMBER);
    }

    int result = libusb_claim_interface(device_handle_, INTERFACE_NUMBER);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to claim interface: " << libusb_error_name(result) << std::endl;
        return Status::ERROR_USB_OPEN_FAILED;
    }

    interface_claimed_ = true;
    return Status::OK;
}

void USBCommunication::releaseInterface() {
    if (device_handle_ && interface_claimed_) {
        libusb_release_interface(device_handle_, INTERFACE_NUMBER);
        interface_claimed_ = false;
    }
}

Status USBCommunication::sendCommand(const uint8_t* command, size_t length) {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (!device_handle_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    int transferred;
    int result = libusb_bulk_transfer(device_handle_, EP_COMMAND_OUT,
                                     const_cast<uint8_t*>(command), length,
                                     &transferred, USB_TIMEOUT_MS);

    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to send command: " << libusb_error_name(result) << std::endl;
        return Status::ERROR_USB_COMM_FAILED;
    }

    return Status::OK;
}

Status USBCommunication::receiveResponse(uint8_t* buffer, size_t length,
                                        size_t& received, int timeout_ms) {
    std::lock_guard<std::mutex> lock(usb_mutex_);

    if (!device_handle_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    int transferred;
    int result = libusb_interrupt_transfer(device_handle_, EP_RESPONSE_IN,
                                          buffer, length, &transferred, timeout_ms);

    received = transferred;

    if (result == LIBUSB_ERROR_TIMEOUT) {
        return Status::ERROR_TIMEOUT;
    } else if (result != LIBUSB_SUCCESS) {
        std::cerr << "Failed to receive response: " << libusb_error_name(result) << std::endl;
        return Status::ERROR_USB_COMM_FAILED;
    }

    return Status::OK;
}

Status USBCommunication::bulkRead(uint8_t endpoint, uint8_t* buffer, size_t length,
                                  size_t& received, int timeout_ms) {
    if (!device_handle_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    int transferred;
    int result = libusb_bulk_transfer(device_handle_, endpoint,
                                     buffer, length, &transferred, timeout_ms);

    received = transferred;

    if (result == LIBUSB_ERROR_TIMEOUT) {
        return Status::ERROR_TIMEOUT;
    } else if (result != LIBUSB_SUCCESS) {
        return Status::ERROR_USB_COMM_FAILED;
    }

    return Status::OK;
}

Status USBCommunication::isochronousRead(uint8_t endpoint, uint8_t* buffer,
                                        size_t length, size_t& received) {
    // TODO: Implement isochronous transfer for depth stream
    // This requires more complex setup with libusb_submit_transfer
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status USBCommunication::readSerialNumber(std::string& serial) {
    if (!device_handle_) {
        return Status::ERROR_DEVICE_NOT_READY;
    }

    libusb_device_descriptor desc;
    libusb_get_device_descriptor(libusb_get_device(device_handle_), &desc);

    unsigned char buffer[256];
    int result = libusb_get_string_descriptor_ascii(device_handle_, desc.iSerialNumber,
                                                   buffer, sizeof(buffer));
    if (result > 0) {
        serial = std::string(reinterpret_cast<char*>(buffer), result);
        return Status::OK;
    }

    return Status::ERROR_USB_COMM_FAILED;
}

Status USBCommunication::readFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build) {
    // TODO: Implement firmware version reading via protocol command
    // For now, set default values
    major = 5;
    minor = 8;
    build = 22;
    return Status::OK;
}

} // namespace xtion
