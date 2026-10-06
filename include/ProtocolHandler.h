#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace xtion {

/**
 * @brief Hardware/firmware versions reported by OPCODE_GET_VERSION
 */
struct VersionInfo {
    uint8_t major;
    uint8_t minor;
    uint16_t build;
    uint32_t chip;            // 0x00202020 = PS1080, 0x00212020 = PS1080A6
    uint16_t fpga;
    uint16_t system_version;
};

/**
 * @brief Protocol handler for Xtion Pro Live commands
 *
 * Implements the PrimeSense PS1080 host protocol (firmware 1.2 and later).
 * Commands and replies are exchanged via vendor control transfers.
 * Reference: OpenNI2 Source/Drivers/PS1080/Sensor/XnHostProtocol.cpp
 */
class ProtocolHandler {
public:
    explicit ProtocolHandler(std::shared_ptr<USBCommunication> usb);
    ~ProtocolHandler() = default;

    /**
     * @brief Get hardware/firmware versions
     * @param version Output version information
     * @return Status code
     */
    Status getVersion(VersionInfo& version);

    /**
     * @brief Get firmware version
     * @param major Major version
     * @param minor Minor version
     * @param build Build number
     * @return Status code
     */
    Status getFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build);

    /**
     * @brief Get sensor serial number (firmware 5.4 and later)
     * @param serial Output serial number
     * @return Status code
     */
    Status getSerialNumber(std::string& serial);

    /**
     * @brief Send keep-alive command
     * @return Status code
     */
    Status keepAlive();

    /**
     * @brief Reset device
     * @return Status code
     */
    Status resetDevice();

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
     * @brief Send a command and wait for its reply
     * @param opcode Operation code
     * @param payload Command payload (must be an even number of bytes)
     * @param payload_size Payload size in bytes
     * @param reply_data Output reply data (without headers)
     * @return Status code
     */
    Status execute(uint16_t opcode, const uint8_t* payload, size_t payload_size,
                   std::vector<uint8_t>& reply_data);

    /**
     * @brief Poll for the reply matching a request
     * @param opcode Expected operation code
     * @param request_id Expected request id
     * @param reply_data Output reply data (without headers)
     * @return Status code
     */
    Status receiveReply(uint16_t opcode, uint16_t request_id,
                        std::vector<uint8_t>& reply_data);

    std::shared_ptr<USBCommunication> usb_;
    std::mutex execute_mutex_;
    uint16_t next_request_id_;

    // Protocol header magic values (firmware 1.2 and later)
    static constexpr uint16_t HOST_MAGIC = 0x4d47;  // "GM"
    static constexpr uint16_t FW_MAGIC = 0x4252;    // "RB"
    static constexpr size_t HEADER_SIZE = 8;        // magic, size, opcode, id
    static constexpr size_t REPLY_HEADER_SIZE = 2;  // error code

    // Command opcodes (firmware 3.0 and later)
    static constexpr uint16_t OPCODE_GET_VERSION = 0;
    static constexpr uint16_t OPCODE_KEEP_ALIVE = 1;
    static constexpr uint16_t OPCODE_GET_PARAM = 2;
    static constexpr uint16_t OPCODE_SET_PARAM = 3;
    static constexpr uint16_t OPCODE_GET_FIXED_PARAMS = 4;
    static constexpr uint16_t OPCODE_GET_SERIAL_NUMBER = 37;

    // Reply error codes
    static constexpr uint16_t ACK = 0;
    static constexpr uint16_t NACK_NOT_READY = 13;

    static constexpr size_t MAX_PACKET_SIZE = 512;  // firmware 5.0 and later
    static constexpr int SEND_RETRIES = 5;
    static constexpr int NOT_READY_RETRIES = 3;
    static constexpr int USB_TIMEOUT_MS = 1000;
    static constexpr int REPLY_TIMEOUT_MS = 2000;
    static constexpr int POST_SEND_DELAY_MS = 10;
    static constexpr int RECEIVE_POLL_MS = 10;
};

} // namespace xtion

#endif // PROTOCOL_HANDLER_H
