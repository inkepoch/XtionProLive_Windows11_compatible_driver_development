#ifndef XTION_COMMON_H
#define XTION_COMMON_H

#include <cstdint>
#include <memory>
#include <string>

namespace xtion {

// Version information
#define XTION_DRIVER_VERSION_MAJOR 1
#define XTION_DRIVER_VERSION_MINOR 0
#define XTION_DRIVER_VERSION_PATCH 0

// USB Device IDs
#define XTION_VENDOR_ID         0x1d27
#define XTION_PRODUCT_ID        0x0601  // Normal mode
#define XTION_PRODUCT_ID_BOOT   0x0600  // Firmware update mode

// USB Endpoints
#define EP_COLOR_IN     0x81  // RGB Color Stream (Bulk)
#define EP_DEPTH_IN     0x82  // Depth Data Stream (Isochronous)
#define EP_IR_IN        0x83  // Infrared Stream (Bulk)
#define EP_COMMAND_OUT  0x02  // Command Output (Control)
#define EP_RESPONSE_IN  0x86  // Response Input (Interrupt)

// Stream resolutions
#define XTION_DEPTH_WIDTH   640
#define XTION_DEPTH_HEIGHT  480
#define XTION_COLOR_WIDTH   640
#define XTION_COLOR_HEIGHT  480
#define XTION_IR_WIDTH      640
#define XTION_IR_HEIGHT     480

// Frame rates
#define XTION_FPS_30    30
#define XTION_FPS_60    60

// Depth values
#define XTION_DEPTH_INVALID     0x0000
#define XTION_DEPTH_MAX         0xFFFF

// Working range
#define XTION_MIN_RANGE_MM  800   // 0.8m
#define XTION_MAX_RANGE_MM  3500  // 3.5m

// Field of view (degrees)
#define XTION_FOV_H     58.0
#define XTION_FOV_V     45.0
#define XTION_FOV_D     70.0

// Required firmware version
#define XTION_REQUIRED_FW_MAJOR 5
#define XTION_REQUIRED_FW_MINOR 8
#define XTION_REQUIRED_FW_BUILD 22

// Status codes
enum class Status {
    OK = 0,
    ERROR_GENERIC = -1,
    ERROR_USB_NOT_FOUND = -2,
    ERROR_USB_OPEN_FAILED = -3,
    ERROR_USB_COMM_FAILED = -4,
    ERROR_DEVICE_NOT_READY = -5,
    ERROR_INVALID_PARAMETER = -6,
    ERROR_TIMEOUT = -7,
    ERROR_OUT_OF_MEMORY = -8,
    ERROR_NOT_IMPLEMENTED = -9,
    ERROR_FIRMWARE_VERSION = -10,
    ERROR_STREAM_NOT_STARTED = -11,
    ERROR_STREAM_ALREADY_STARTED = -12
};

// Pixel formats
enum class PixelFormat {
    DEPTH_1MM = 0,      // 16-bit depth in millimeters
    RGB888 = 1,         // 24-bit RGB
    YUV422 = 2,         // YUV 4:2:2
    GRAY8 = 3,          // 8-bit grayscale
    GRAY16 = 4          // 16-bit grayscale (IR)
};

// Stream type
enum class StreamType {
    DEPTH = 0,
    COLOR = 1,
    IR = 2
};

// Device information
struct DeviceInfo {
    uint16_t vendor_id;
    uint16_t product_id;
    std::string serial_number;
    std::string usb_path;
    uint8_t fw_version_major;
    uint8_t fw_version_minor;
    uint16_t fw_version_build;
};

// Frame metadata
struct FrameMetadata {
    uint64_t timestamp_us;      // Timestamp in microseconds
    uint32_t frame_index;       // Frame sequence number
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    uint32_t stride;            // Bytes per row
    uint32_t data_size;         // Total data size in bytes
};

// Depth frame
struct DepthFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint16_t[]> data;  // Depth data in millimeters
};

// Color frame
struct ColorFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint8_t[]> data;   // Color data (RGB888 or YUV422)
};

// IR frame
struct IRFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint16_t[]> data;  // IR intensity data
};

// Helper functions
inline const char* statusToString(Status status) {
    switch (status) {
        case Status::OK: return "OK";
        case Status::ERROR_GENERIC: return "Generic error";
        case Status::ERROR_USB_NOT_FOUND: return "USB device not found";
        case Status::ERROR_USB_OPEN_FAILED: return "Failed to open USB device";
        case Status::ERROR_USB_COMM_FAILED: return "USB communication failed";
        case Status::ERROR_DEVICE_NOT_READY: return "Device not ready";
        case Status::ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case Status::ERROR_TIMEOUT: return "Timeout";
        case Status::ERROR_OUT_OF_MEMORY: return "Out of memory";
        case Status::ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case Status::ERROR_FIRMWARE_VERSION: return "Incompatible firmware version";
        case Status::ERROR_STREAM_NOT_STARTED: return "Stream not started";
        case Status::ERROR_STREAM_ALREADY_STARTED: return "Stream already started";
        default: return "Unknown error";
    }
}

} // namespace xtion

#endif // XTION_COMMON_H
