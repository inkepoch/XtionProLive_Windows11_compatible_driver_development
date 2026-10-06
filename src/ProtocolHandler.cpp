#include "ProtocolHandler.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

namespace xtion {

namespace {

uint16_t readLE16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t readLE32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void writeLE16(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value & 0xFF);
    p[1] = static_cast<uint8_t>(value >> 8);
}

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace

ProtocolHandler::ProtocolHandler(std::shared_ptr<USBCommunication> usb)
    : usb_(usb)
    , next_request_id_(0) {
}

Status ProtocolHandler::getVersion(VersionInfo& version) {
    std::vector<uint8_t> data;
    Status status = execute(OPCODE_GET_VERSION, nullptr, 0, data);
    if (status != Status::OK) {
        return status;
    }

    if (data.size() < 12) {
        std::cerr << "GetVersion reply too short: " << data.size() << " bytes" << std::endl;
        return Status::ERROR_PROTOCOL;
    }

    // The first 16-bit word is byte-swapped: data[0] = minor, data[1] = major
    version.major = data[1];
    version.minor = data[0];
    version.build = readLE16(&data[2]);
    version.chip = readLE32(&data[4]);
    version.fpga = readLE16(&data[8]);
    version.system_version = readLE16(&data[10]);

    // From firmware 5.x the build number is BCD-like (0x22 means 22)
    if (version.major >= 5) {
        char hex[8];
        std::snprintf(hex, sizeof(hex), "%x", version.build);
        version.build = static_cast<uint16_t>(std::atoi(hex));
    }

    return Status::OK;
}

Status ProtocolHandler::getFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build) {
    VersionInfo version{};
    Status status = getVersion(version);
    if (status != Status::OK) {
        return status;
    }

    major = version.major;
    minor = version.minor;
    build = version.build;
    return Status::OK;
}

Status ProtocolHandler::getSerialNumber(std::string& serial) {
    std::vector<uint8_t> data;
    Status status = execute(OPCODE_GET_SERIAL_NUMBER, nullptr, 0, data);
    if (status != Status::OK) {
        return status;
    }

    // Null-terminated ASCII string padded to a 16-bit boundary
    serial.assign(data.begin(), data.end());
    size_t end = serial.find('\0');
    if (end != std::string::npos) {
        serial.resize(end);
    }
    return Status::OK;
}

Status ProtocolHandler::keepAlive() {
    std::vector<uint8_t> data;
    return execute(OPCODE_KEEP_ALIVE, nullptr, 0, data);
}

// The following commands do not exist as dedicated opcodes on the PS1080.
// Streams are configured and started via OPCODE_SET_PARAM (see OpenNI2
// XnHostProtocolSetParam / PARAM_GENERAL_STREAMx_MODE). Until that is
// implemented they return ERROR_NOT_IMPLEMENTED instead of sending
// invalid commands to the device.

Status ProtocolHandler::resetDevice() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::initializeDepthCamera() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::initializeRGBCamera() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::initializeIRCamera() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::startDepthStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::stopDepthStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::startColorStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::stopColorStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::startIRStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::stopIRStream() {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::setStreamMode(uint8_t /*mode*/) {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::getTemperature(float& /*temperature*/) {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::setDepthRegistration(bool /*enable*/) {
    return Status::ERROR_NOT_IMPLEMENTED;
}

Status ProtocolHandler::execute(uint16_t opcode, const uint8_t* payload, size_t payload_size,
                                std::vector<uint8_t>& reply_data) {
    if (payload_size % 2 != 0 || HEADER_SIZE + payload_size > MAX_PACKET_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(execute_mutex_);

    uint8_t request[MAX_PACKET_SIZE] = {};
    uint16_t request_id = next_request_id_++;
    writeLE16(&request[0], HOST_MAGIC);
    writeLE16(&request[2], static_cast<uint16_t>(payload_size / 2));
    writeLE16(&request[4], opcode);
    writeLE16(&request[6], request_id);
    if (payload_size > 0) {
        std::memcpy(&request[HEADER_SIZE], payload, payload_size);
    }
    size_t request_size = HEADER_SIZE + payload_size;

    Status status = Status::ERROR_DEVICE_NOT_READY;
    for (int attempt = 0; attempt < NOT_READY_RETRIES; attempt++) {
        // Send request (device may NAK/stall while busy)
        for (int retry = 0; retry < SEND_RETRIES; retry++) {
            status = usb_->sendCommand(request, request_size, USB_TIMEOUT_MS);
            if (status != Status::ERROR_TIMEOUT) {
                break;
            }
            sleepMs(100);
        }
        if (status != Status::OK) {
            return status;
        }

        sleepMs(POST_SEND_DELAY_MS);

        status = receiveReply(opcode, request_id, reply_data);
        if (status != Status::ERROR_DEVICE_NOT_READY) {
            return status;
        }

        sleepMs(1000);
    }

    return status;
}

Status ProtocolHandler::receiveReply(uint16_t opcode, uint16_t request_id,
                                     std::vector<uint8_t>& reply_data) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(REPLY_TIMEOUT_MS);
    uint8_t buffer[MAX_PACKET_SIZE];

    while (std::chrono::steady_clock::now() < deadline) {
        size_t received = 0;
        Status status = usb_->receiveResponse(buffer, sizeof(buffer), received, USB_TIMEOUT_MS);
        if (status == Status::ERROR_TIMEOUT || (status == Status::OK && received == 0)) {
            sleepMs(RECEIVE_POLL_MS);
            continue;
        }
        if (status != Status::OK) {
            return status;
        }

        // Locate the reply magic
        size_t offset = 0;
        while (offset + HEADER_SIZE + REPLY_HEADER_SIZE <= received &&
               readLE16(&buffer[offset]) != FW_MAGIC) {
            offset++;
        }
        if (offset + HEADER_SIZE + REPLY_HEADER_SIZE > received) {
            sleepMs(RECEIVE_POLL_MS);
            continue;
        }

        const uint8_t* header = &buffer[offset];
        uint16_t size_words = readLE16(&header[2]);
        uint16_t reply_opcode = readLE16(&header[4]);
        uint16_t reply_id = readLE16(&header[6]);

        if (reply_id != request_id) {
            continue;  // Stale reply to an earlier request
        }
        if (reply_opcode != opcode) {
            std::cerr << "Unexpected reply opcode " << reply_opcode
                      << " (expected " << opcode << ")" << std::endl;
            return Status::ERROR_PROTOCOL;
        }

        uint16_t error_code = readLE16(&header[HEADER_SIZE]);
        if (error_code == NACK_NOT_READY) {
            return Status::ERROR_DEVICE_NOT_READY;
        }
        if (error_code != ACK) {
            std::cerr << "Device returned NACK " << error_code
                      << " for opcode " << opcode << std::endl;
            return Status::ERROR_PROTOCOL;
        }
        if (size_words == 0) {
            return Status::ERROR_PROTOCOL;
        }

        // Size field counts 16-bit words including the error code
        size_t data_size = (static_cast<size_t>(size_words) - 1) * 2;
        size_t data_offset = offset + HEADER_SIZE + REPLY_HEADER_SIZE;
        reply_data.assign(buffer + data_offset, buffer + received);

        // Large replies continue in following packets
        while (reply_data.size() < data_size) {
            size_t more = 0;
            status = usb_->receiveResponse(buffer, sizeof(buffer), more, USB_TIMEOUT_MS);
            if (status != Status::OK) {
                return status;
            }
            if (more == 0) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    return Status::ERROR_TIMEOUT;
                }
                sleepMs(RECEIVE_POLL_MS);
                continue;
            }
            reply_data.insert(reply_data.end(), buffer, buffer + more);
        }
        reply_data.resize(data_size);

        return Status::OK;
    }

    return Status::ERROR_TIMEOUT;
}

} // namespace xtion
