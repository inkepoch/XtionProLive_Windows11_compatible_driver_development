# Xtion Pro Live Driver API Documentation

## Overview

This document describes the C++ API for the Xtion Pro Live driver.

## Core Classes

### XtionDevice

Main interface for interacting with the Xtion Pro Live sensor.

#### Basic Usage

```cpp
#include "XtionDevice.h"

// Create device
xtion::XtionDevice device;

// Open device
xtion::Status status = device.open(0);
if (status != xtion::Status::OK) {
    // Handle error
}

// Start depth stream
device.startDepthStream();

// Read frames
xtion::DepthFrame frame;
device.readDepthFrame(frame, 1000);

// Cleanup
device.stopDepthStream();
device.close();
```

#### Methods

##### Device Management

```cpp
// Enumerate all connected devices
static Status enumerateDevices(std::vector<DeviceInfo>& devices);

// Open device connection
Status open(int device_index = 0);

// Close device connection
void close();

// Check if device is open
bool isOpen() const;

// Get device information
const DeviceInfo& getDeviceInfo() const;
```

##### Depth Stream

```cpp
// Start depth stream
Status startDepthStream();

// Stop depth stream
void stopDepthStream();

// Check if depth stream is running
bool isDepthStreamRunning() const;

// Read depth frame
Status readDepthFrame(DepthFrame& frame, int timeout_ms = 1000);

// Set depth range (in millimeters)
void setDepthRange(uint16_t min_mm, uint16_t max_mm);
```

##### Color Stream

```cpp
// Start color stream
Status startColorStream();

// Stop color stream
void stopColorStream();

// Check if color stream is running
bool isColorStreamRunning() const;

// Read color frame
Status readColorFrame(ColorFrame& frame, int timeout_ms = 1000);

// Set pixel format
void setColorPixelFormat(PixelFormat format);
```

##### IR Stream

```cpp
// Start IR stream
Status startIRStream();

// Stop IR stream
void stopIRStream();

// Check if IR stream is running
bool isIRStreamRunning() const;

// Read IR frame
Status readIRFrame(IRFrame& frame, int timeout_ms = 1000);
```

##### Advanced Settings

```cpp
// Enable/disable depth-to-color registration
Status setRegistration(bool enable);

// Set mirroring for all streams
void setMirroring(bool enable);

// Get device temperature
Status getTemperature(float& temperature);
```

## Data Structures

### Status

Status codes returned by API functions.

```cpp
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

// Convert status to string
const char* statusToString(Status status);
```

### DeviceInfo

Information about a connected device.

```cpp
struct DeviceInfo {
    uint16_t vendor_id;
    uint16_t product_id;
    std::string serial_number;
    std::string usb_path;
    uint8_t fw_version_major;
    uint8_t fw_version_minor;
    uint16_t fw_version_build;
};
```

### FrameMetadata

Metadata for captured frames.

```cpp
struct FrameMetadata {
    uint64_t timestamp_us;      // Timestamp in microseconds
    uint32_t frame_index;       // Frame sequence number
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    uint32_t stride;            // Bytes per row
    uint32_t data_size;         // Total data size in bytes
};
```

### DepthFrame

Depth frame data.

```cpp
struct DepthFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint16_t[]> data;  // Depth data in millimeters
};

// Access depth value at (x, y)
uint16_t depth = frame.data[y * frame.metadata.width + x];

// Check if valid
if (depth != XTION_DEPTH_INVALID && depth > 0) {
    // Valid depth value in millimeters
}
```

### ColorFrame

Color frame data.

```cpp
struct ColorFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint8_t[]> data;   // Color data (RGB888 or YUV422)
};

// Access RGB pixel at (x, y) - assuming RGB888 format
int offset = (y * frame.metadata.width + x) * 3;
uint8_t r = frame.data[offset + 0];
uint8_t g = frame.data[offset + 1];
uint8_t b = frame.data[offset + 2];
```

### IRFrame

Infrared frame data.

```cpp
struct IRFrame {
    FrameMetadata metadata;
    std::shared_ptr<uint16_t[]> data;  // IR intensity data
};

// Access IR value at (x, y)
uint16_t intensity = frame.data[y * frame.metadata.width + x];
```

### PixelFormat

Supported pixel formats.

```cpp
enum class PixelFormat {
    DEPTH_1MM = 0,      // 16-bit depth in millimeters
    RGB888 = 1,         // 24-bit RGB
    YUV422 = 2,         // YUV 4:2:2
    GRAY8 = 3,          // 8-bit grayscale
    GRAY16 = 4          // 16-bit grayscale (IR)
};
```

## Constants

```cpp
// Device IDs
#define XTION_VENDOR_ID         0x1d27
#define XTION_PRODUCT_ID        0x0601

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
```

## Examples

### Example 1: Simple Depth Capture

```cpp
#include "XtionDevice.h"
#include <iostream>

int main() {
    xtion::XtionDevice device;

    if (device.open() != xtion::Status::OK) {
        std::cerr << "Failed to open device" << std::endl;
        return 1;
    }

    if (device.startDepthStream() != xtion::Status::OK) {
        std::cerr << "Failed to start depth stream" << std::endl;
        device.close();
        return 1;
    }

    // Capture 100 frames
    for (int i = 0; i < 100; i++) {
        xtion::DepthFrame frame;
        if (device.readDepthFrame(frame) == xtion::Status::OK) {
            // Process frame
            std::cout << "Frame " << i << " captured" << std::endl;
        }
    }

    device.stopDepthStream();
    device.close();

    return 0;
}
```

### Example 2: Enumerate Devices

```cpp
#include "XtionDevice.h"
#include <iostream>

int main() {
    std::vector<xtion::DeviceInfo> devices;

    if (xtion::XtionDevice::enumerateDevices(devices) == xtion::Status::OK) {
        std::cout << "Found " << devices.size() << " device(s)" << std::endl;

        for (size_t i = 0; i < devices.size(); i++) {
            std::cout << "Device " << i << ":" << std::endl;
            std::cout << "  Serial: " << devices[i].serial_number << std::endl;
            std::cout << "  Firmware: "
                     << (int)devices[i].fw_version_major << "."
                     << (int)devices[i].fw_version_minor << "."
                     << devices[i].fw_version_build << std::endl;
        }
    }

    return 0;
}
```

### Example 3: Depth and Color Capture

```cpp
#include "XtionDevice.h"
#include <iostream>

int main() {
    xtion::XtionDevice device;

    device.open();
    device.startDepthStream();
    device.startColorStream();

    while (true) {
        xtion::DepthFrame depth_frame;
        xtion::ColorFrame color_frame;

        device.readDepthFrame(depth_frame, 100);
        device.readColorFrame(color_frame, 100);

        // Process frames...
    }

    device.stopDepthStream();
    device.stopColorStream();
    device.close();

    return 0;
}
```

## Error Handling

Always check return values:

```cpp
xtion::Status status = device.open();
if (status != xtion::Status::OK) {
    std::cerr << "Error: " << xtion::statusToString(status) << std::endl;
    // Handle error appropriately
}
```

## Thread Safety

- `XtionDevice` is NOT thread-safe
- Use separate device instances for different threads
- Or use external synchronization (mutex)

## Performance Tips

1. Use non-blocking reads when possible:
```cpp
device.readDepthFrame(frame, 0);  // Non-blocking
```

2. Reuse frame objects:
```cpp
xtion::DepthFrame frame;
while (running) {
    device.readDepthFrame(frame);
    // Process frame...
}
```

3. Set appropriate depth range:
```cpp
device.setDepthRange(800, 2000);  // Only 0.8m - 2.0m
```

4. Process frames in separate thread:
```cpp
std::thread capture_thread([&]() {
    while (running) {
        device.readDepthFrame(frame);
        queue.push(frame);
    }
});
```
