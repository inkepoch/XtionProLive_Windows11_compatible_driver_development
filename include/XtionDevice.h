#ifndef XTION_DEVICE_H
#define XTION_DEVICE_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include "ProtocolHandler.h"
#include "DepthStream.h"
#include "ColorStream.h"
#include "IRStream.h"
#include <memory>
#include <vector>

namespace xtion {

/**
 * @brief Main interface for Xtion Pro Live device
 *
 * This is the primary class for interacting with the Xtion Pro Live sensor.
 * It manages device connection, streams, and provides a high-level API.
 */
class XtionDevice {
public:
    XtionDevice();
    ~XtionDevice();

    // Prevent copying
    XtionDevice(const XtionDevice&) = delete;
    XtionDevice& operator=(const XtionDevice&) = delete;

    /**
     * @brief Enumerate all connected Xtion devices
     * @param devices Output vector of device information
     * @return Status code
     */
    static Status enumerateDevices(std::vector<DeviceInfo>& devices);

    /**
     * @brief Open device connection
     * @param device_index Device index (default: 0)
     * @return Status code
     */
    Status open(int device_index = 0);

    /**
     * @brief Close device connection
     */
    void close();

    /**
     * @brief Check if device is open
     * @return true if open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Get device information
     * @return Device info structure
     */
    const DeviceInfo& getDeviceInfo() const;

    // Depth stream methods
    /**
     * @brief Start depth stream
     * @return Status code
     */
    Status startDepthStream();

    /**
     * @brief Stop depth stream
     */
    void stopDepthStream();

    /**
     * @brief Check if depth stream is running
     * @return true if running, false otherwise
     */
    bool isDepthStreamRunning() const;

    /**
     * @brief Read depth frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds
     * @return Status code
     */
    Status readDepthFrame(DepthFrame& frame, int timeout_ms = 1000);

    /**
     * @brief Set depth range
     * @param min_mm Minimum range in millimeters
     * @param max_mm Maximum range in millimeters
     */
    void setDepthRange(uint16_t min_mm, uint16_t max_mm);

    // Color stream methods
    /**
     * @brief Start color stream
     * @return Status code
     */
    Status startColorStream();

    /**
     * @brief Stop color stream
     */
    void stopColorStream();

    /**
     * @brief Check if color stream is running
     * @return true if running, false otherwise
     */
    bool isColorStreamRunning() const;

    /**
     * @brief Read color frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds
     * @return Status code
     */
    Status readColorFrame(ColorFrame& frame, int timeout_ms = 1000);

    /**
     * @brief Set color pixel format
     * @param format Pixel format
     */
    void setColorPixelFormat(PixelFormat format);

    // IR stream methods
    /**
     * @brief Start IR stream
     * @return Status code
     */
    Status startIRStream();

    /**
     * @brief Stop IR stream
     */
    void stopIRStream();

    /**
     * @brief Check if IR stream is running
     * @return true if running, false otherwise
     */
    bool isIRStreamRunning() const;

    /**
     * @brief Read IR frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds
     * @return Status code
     */
    Status readIRFrame(IRFrame& frame, int timeout_ms = 1000);

    // Advanced settings
    /**
     * @brief Enable/disable depth-to-color registration
     * @param enable Registration flag
     * @return Status code
     */
    Status setRegistration(bool enable);

    /**
     * @brief Set mirroring for all streams
     * @param enable Mirror flag
     */
    void setMirroring(bool enable);

    /**
     * @brief Get device temperature
     * @param temperature Output temperature in Celsius
     * @return Status code
     */
    Status getTemperature(float& temperature);

private:
    /**
     * @brief Initialize device after opening
     * @return Status code
     */
    Status initialize();

    /**
     * @brief Verify firmware version
     * @return Status code
     */
    Status verifyFirmwareVersion();

    std::shared_ptr<USBCommunication> usb_;
    std::shared_ptr<ProtocolHandler> protocol_;
    std::unique_ptr<DepthStream> depth_stream_;
    std::unique_ptr<ColorStream> color_stream_;
    std::unique_ptr<IRStream> ir_stream_;

    bool is_open_;
    bool is_initialized_;
};

} // namespace xtion

#endif // XTION_DEVICE_H
