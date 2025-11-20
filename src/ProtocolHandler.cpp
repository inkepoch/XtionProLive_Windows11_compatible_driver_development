#include "ProtocolHandler.h"
#include <cstring>
#include <iostream>

namespace xtion {

ProtocolHandler::ProtocolHandler(std::shared_ptr<USBCommunication> usb)
    : usb_(usb) {
}

Status ProtocolHandler::resetDevice() {
    uint8_t command[6] = {CMD_RESET, 0x00, 0x00, 0x01, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::getFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build) {
    uint8_t command[6] = {CMD_GET_VERSION, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t response[MAX_RESPONSE_SIZE];
    size_t received = 0;

    Status status = sendCommandWithResponse(command, sizeof(command),
                                           response, sizeof(response), received);
    if (status != Status::OK) {
        return status;
    }

    // Parse version from response
    // TODO: Implement actual parsing based on protocol specification
    if (received >= 6) {
        major = response[2];
        minor = response[3];
        build = (response[4] << 8) | response[5];
    }

    return Status::OK;
}

Status ProtocolHandler::initializeDepthCamera() {
    uint8_t command[6] = {CMD_DEPTH_INIT, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::initializeRGBCamera() {
    uint8_t command[6] = {CMD_RGB_INIT, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::initializeIRCamera() {
    uint8_t command[6] = {CMD_IR_INIT, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::startDepthStream() {
    uint8_t command[6] = {CMD_START_DEPTH, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::stopDepthStream() {
    uint8_t command[6] = {CMD_STOP_DEPTH, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::startColorStream() {
    uint8_t command[6] = {CMD_START_COLOR, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::stopColorStream() {
    uint8_t command[6] = {CMD_STOP_COLOR, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::startIRStream() {
    uint8_t command[6] = {CMD_START_IR, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::stopIRStream() {
    uint8_t command[6] = {CMD_STOP_IR, 0x00, 0x00, 0x00, 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::setStreamMode(uint8_t mode) {
    uint8_t command[6] = {CMD_SET_MODE, 0x00, 0x02, 0x03, mode, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::getTemperature(float& temperature) {
    uint8_t command[6] = {CMD_GET_TEMP, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t response[MAX_RESPONSE_SIZE];
    size_t received = 0;

    Status status = sendCommandWithResponse(command, sizeof(command),
                                           response, sizeof(response), received);
    if (status != Status::OK) {
        return status;
    }

    // Parse temperature from response
    // TODO: Implement actual parsing based on protocol specification
    if (received >= 4) {
        int16_t temp_raw = (response[2] << 8) | response[3];
        temperature = temp_raw / 10.0f;  // Convert to Celsius
    }

    return Status::OK;
}

Status ProtocolHandler::setDepthRegistration(bool enable) {
    uint8_t command[6] = {CMD_SET_REGISTRATION, 0x00, 0x00, 0x00, enable ? 0x01 : 0x00, 0x00};
    return usb_->sendCommand(command, sizeof(command));
}

Status ProtocolHandler::sendCommandWithResponse(const uint8_t* command, size_t cmd_length,
                                               uint8_t* response, size_t resp_length,
                                               size_t& received) {
    Status status = usb_->sendCommand(command, cmd_length);
    if (status != Status::OK) {
        return status;
    }

    // Wait for response
    return usb_->receiveResponse(response, resp_length, received, 1000);
}

size_t ProtocolHandler::buildCommand(uint8_t opcode, const uint16_t* params,
                                    size_t param_count, uint8_t* buffer) {
    buffer[0] = opcode;
    buffer[1] = 0x00;  // Reserved

    size_t offset = 2;
    for (size_t i = 0; i < param_count; i++) {
        buffer[offset++] = (params[i] >> 8) & 0xFF;
        buffer[offset++] = params[i] & 0xFF;
    }

    // Pad to minimum command size
    while (offset < 6) {
        buffer[offset++] = 0x00;
    }

    return offset;
}

} // namespace xtion
