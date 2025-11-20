#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include <memory>

namespace xtion {

/**
 * @brief Protocol handler for Xtion Pro Live commands
 *
 * Implements the PrimeSense PS1080 protocol for device control.
 * Based on analysis from xqms/xtion and libfreenect projects.
 */
class ProtocolHandler {
public:
    explicit ProtocolHandler(std::shared_ptr<USBCommunication> usb);
    ~ProtocolHandler() = default;

    /**
     * @brief Reset device
     * @return Status code
     */
    Status resetDevice();

    /**
     * @brief Get firmware version
     * @param major Major version
     * @param minor Minor version
     * @param build Build number
     * @return Status code
     */
    Status getFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build);

    /**
     * @brief Initialize depth camera
     * @return Status code
     */
    Status initializeDepthCamera();

    /**
     * @brief Initialize RGB camera
     * @return Status code
     */
    Status initializeRGBCamera();

    /**
     * @brief Initialize IR camera
     * @return Status code
     */
    Status initializeIRCamera();

    /**
     * @brief Start depth stream
     * @return Status code
     */
    Status startDepthStream();

    /**
     * @brief Stop depth stream
     * @return Status code
     */
    Status stopDepthStream();

    /**
     * @brief Start color stream
     * @return Status code
     */
    Status startColorStream();

    /**
     * @brief Stop color stream
     * @return Status code
     */
    Status stopColorStream();

    /**
     * @brief Start IR stream
     * @return Status code
     */
    Status startIRStream();

    /**
     * @brief Stop IR stream
     * @return Status code
     */
    Status stopIRStream();

    /**
     * @brief Set stream mode
     * @param mode Stream mode value
     * @return Status code
     */
    Status setStreamMode(uint8_t mode);

    /**
     * @brief Get device temperature
     * @param temperature Temperature in Celsius
     * @return Status code
     */
    Status getTemperature(float& temperature);

    /**
     * @brief Set depth registration (align depth to color)
     * @param enable Enable/disable registration
     * @return Status code
     */
    Status setDepthRegistration(bool enable);

private:
    /**
     * @brief Send command and wait for response
     * @param command Command buffer
     * @param cmd_length Command length
     * @param response Response buffer
     * @param resp_length Maximum response length
     * @param received Actual bytes received
     * @return Status code
     */
    Status sendCommandWithResponse(const uint8_t* command, size_t cmd_length,
                                   uint8_t* response, size_t resp_length,
                                   size_t& received);

    /**
     * @brief Build command packet
     * @param opcode Operation code
     * @param params Parameters
     * @param param_count Parameter count
     * @param buffer Output buffer
     * @return Command length
     */
    size_t buildCommand(uint8_t opcode, const uint16_t* params,
                       size_t param_count, uint8_t* buffer);

    std::shared_ptr<USBCommunication> usb_;

    // Command opcodes (based on reverse engineering)
    static constexpr uint8_t CMD_RESET = 0x05;
    static constexpr uint8_t CMD_GET_VERSION = 0x02;
    static constexpr uint8_t CMD_SET_MODE = 0x05;
    static constexpr uint8_t CMD_DEPTH_INIT = 0x13;
    static constexpr uint8_t CMD_RGB_INIT = 0x14;
    static constexpr uint8_t CMD_IR_INIT = 0x15;
    static constexpr uint8_t CMD_START_DEPTH = 0x20;
    static constexpr uint8_t CMD_STOP_DEPTH = 0x21;
    static constexpr uint8_t CMD_START_COLOR = 0x22;
    static constexpr uint8_t CMD_STOP_COLOR = 0x23;
    static constexpr uint8_t CMD_START_IR = 0x24;
    static constexpr uint8_t CMD_STOP_IR = 0x25;
    static constexpr uint8_t CMD_GET_TEMP = 0x30;
    static constexpr uint8_t CMD_SET_REGISTRATION = 0x40;

    static constexpr size_t MAX_COMMAND_SIZE = 1024;
    static constexpr size_t MAX_RESPONSE_SIZE = 1024;
};

} // namespace xtion

#endif // PROTOCOL_HANDLER_H
