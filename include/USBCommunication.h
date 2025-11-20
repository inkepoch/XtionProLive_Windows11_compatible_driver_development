#ifndef USB_COMMUNICATION_H
#define USB_COMMUNICATION_H

#include "XtionCommon.h"
#include <libusb.h>
#include <vector>
#include <mutex>

namespace xtion {

/**
 * @brief USB communication handler for Xtion Pro Live
 *
 * Manages low-level USB communication with the device using libusb.
 */
class USBCommunication {
public:
    USBCommunication();
    ~USBCommunication();

    // Prevent copying
    USBCommunication(const USBCommunication&) = delete;
    USBCommunication& operator=(const USBCommunication&) = delete;

    /**
     * @brief Initialize libusb context
     * @return Status code
     */
    Status initialize();

    /**
     * @brief Shutdown and cleanup libusb
     */
    void shutdown();

    /**
     * @brief Find and enumerate all Xtion devices
     * @param devices Output vector of device information
     * @return Status code
     */
    Status enumerateDevices(std::vector<DeviceInfo>& devices);

    /**
     * @brief Open connection to a specific device
     * @param device_index Index in enumerated devices list
     * @return Status code
     */
    Status open(int device_index = 0);

    /**
     * @brief Close current device connection
     */
    void close();

    /**
     * @brief Check if device is currently open
     * @return true if open, false otherwise
     */
    bool isOpen() const { return device_handle_ != nullptr; }

    /**
     * @brief Send control command to device
     * @param command Command buffer
     * @param length Command length
     * @return Status code
     */
    Status sendCommand(const uint8_t* command, size_t length);

    /**
     * @brief Receive response from device
     * @param buffer Response buffer
     * @param length Maximum buffer length
     * @param received Actual bytes received
     * @param timeout_ms Timeout in milliseconds
     * @return Status code
     */
    Status receiveResponse(uint8_t* buffer, size_t length,
                          size_t& received, int timeout_ms = 1000);

    /**
     * @brief Read bulk data from endpoint
     * @param endpoint Endpoint address
     * @param buffer Data buffer
     * @param length Buffer length
     * @param received Actual bytes received
     * @param timeout_ms Timeout in milliseconds
     * @return Status code
     */
    Status bulkRead(uint8_t endpoint, uint8_t* buffer, size_t length,
                   size_t& received, int timeout_ms = 1000);

    /**
     * @brief Read isochronous data from endpoint
     * @param endpoint Endpoint address
     * @param buffer Data buffer
     * @param length Buffer length
     * @param received Actual bytes received
     * @return Status code
     */
    Status isochronousRead(uint8_t endpoint, uint8_t* buffer,
                          size_t length, size_t& received);

    /**
     * @brief Get current device information
     * @return Device info structure
     */
    const DeviceInfo& getDeviceInfo() const { return device_info_; }

private:
    /**
     * @brief Claim device interface
     * @return Status code
     */
    Status claimInterface();

    /**
     * @brief Release device interface
     */
    void releaseInterface();

    /**
     * @brief Read device serial number
     * @param serial Output serial number string
     * @return Status code
     */
    Status readSerialNumber(std::string& serial);

    /**
     * @brief Read firmware version
     * @param major Major version
     * @param minor Minor version
     * @param build Build number
     * @return Status code
     */
    Status readFirmwareVersion(uint8_t& major, uint8_t& minor, uint16_t& build);

    libusb_context* context_;
    libusb_device_handle* device_handle_;
    DeviceInfo device_info_;
    bool interface_claimed_;
    std::mutex usb_mutex_;

    static constexpr int INTERFACE_NUMBER = 0;
    static constexpr int USB_TIMEOUT_MS = 1000;
};

} // namespace xtion

#endif // USB_COMMUNICATION_H
