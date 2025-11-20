#include "ColorStream.h"
#include <chrono>
#include <cstring>
#include <iostream>

namespace xtion {

ColorStream::ColorStream(std::shared_ptr<USBCommunication> usb)
    : usb_(usb)
    , is_running_(false)
    , pixel_format_(PixelFormat::RGB888)
    , mirror_enabled_(false)
    , auto_exposure_(true)
    , auto_white_balance_(true)
    , current_fps_(0.0f)
    , last_frame_time_(0)
    , frame_counter_(0) {
}

ColorStream::~ColorStream() {
    stop();
}

Status ColorStream::start() {
    if (is_running_) {
        return Status::ERROR_STREAM_ALREADY_STARTED;
    }

    // Clear frame queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!frame_queue_.empty()) {
            frame_queue_.pop();
        }
    }

    is_running_ = true;
    acquisition_thread_ = std::thread(&ColorStream::acquisitionThread, this);

    return Status::OK;
}

void ColorStream::stop() {
    if (!is_running_) {
        return;
    }

    is_running_ = false;

    // Wake up waiting threads
    queue_cv_.notify_all();

    // Wait for acquisition thread to finish
    if (acquisition_thread_.joinable()) {
        acquisition_thread_.join();
    }

    // Clear frame queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!frame_queue_.empty()) {
            frame_queue_.pop();
        }
    }
}

Status ColorStream::readFrame(ColorFrame& frame, int timeout_ms) {
    if (!is_running_) {
        return Status::ERROR_STREAM_NOT_STARTED;
    }

    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (timeout_ms == 0) {
        // Non-blocking read
        if (frame_queue_.empty()) {
            return Status::ERROR_TIMEOUT;
        }
    } else {
        // Blocking read with timeout
        auto timeout = std::chrono::milliseconds(timeout_ms);
        if (!queue_cv_.wait_for(lock, timeout, [this] { return !frame_queue_.empty() || !is_running_; })) {
            return Status::ERROR_TIMEOUT;
        }

        if (!is_running_) {
            return Status::ERROR_STREAM_NOT_STARTED;
        }
    }

    if (frame_queue_.empty()) {
        return Status::ERROR_TIMEOUT;
    }

    frame = std::move(frame_queue_.front());
    frame_queue_.pop();

    return Status::OK;
}

void ColorStream::setPixelFormat(PixelFormat format) {
    if (format == PixelFormat::RGB888 || format == PixelFormat::YUV422) {
        pixel_format_ = format;
    }
}

void ColorStream::setAutoExposure(bool enable) {
    auto_exposure_ = enable;
    // TODO: Send command to device to enable/disable auto exposure
}

void ColorStream::setAutoWhiteBalance(bool enable) {
    auto_white_balance_ = enable;
    // TODO: Send command to device to enable/disable auto white balance
}

void ColorStream::acquisitionThread() {
    std::vector<uint8_t> buffer(COLOR_BUFFER_SIZE);

    while (is_running_) {
        size_t received = 0;
        Status status = usb_->bulkRead(EP_COLOR_IN, buffer.data(), buffer.size(), received, 100);

        if (status == Status::ERROR_TIMEOUT) {
            continue;
        } else if (status != Status::OK) {
            std::cerr << "Color stream read error: " << statusToString(status) << std::endl;
            continue;
        }

        if (received == 0) {
            continue;
        }

        // Process raw data into color frame
        ColorFrame frame = processRawData(buffer.data(), received);

        // Add to queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (frame_queue_.size() >= MAX_QUEUE_SIZE) {
                frame_queue_.pop();  // Drop oldest frame
            }
            frame_queue_.push(std::move(frame));
        }

        queue_cv_.notify_one();

        // Update FPS
        updateFPS();
    }
}

ColorFrame ColorStream::processRawData(const uint8_t* raw_data, size_t size) {
    ColorFrame frame;

    // Set metadata
    frame.metadata.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    frame.metadata.frame_index = frame_counter_++;
    frame.metadata.width = XTION_COLOR_WIDTH;
    frame.metadata.height = XTION_COLOR_HEIGHT;
    frame.metadata.format = pixel_format_;
    frame.metadata.stride = XTION_COLOR_WIDTH * 3;  // RGB888
    frame.metadata.data_size = XTION_COLOR_WIDTH * XTION_COLOR_HEIGHT * 3;

    // Allocate data buffer
    frame.data = std::shared_ptr<uint8_t[]>(new uint8_t[frame.metadata.data_size]);

    // Convert raw data
    // TODO: Implement actual conversion based on protocol specification
    // Check if raw data is YUV422 and convert to RGB888 if needed
    if (size >= XTION_COLOR_WIDTH * XTION_COLOR_HEIGHT * 2) {
        // Assume YUV422 input
        convertYUV422toRGB888(raw_data, frame.data.get(), XTION_COLOR_WIDTH * XTION_COLOR_HEIGHT);
    } else {
        // Simple copy
        size_t copy_size = std::min(size, frame.metadata.data_size);
        std::memcpy(frame.data.get(), raw_data, copy_size);
    }

    // Mirror if enabled
    if (mirror_enabled_) {
        uint8_t* data = frame.data.get();
        for (int y = 0; y < XTION_COLOR_HEIGHT; y++) {
            for (int x = 0; x < XTION_COLOR_WIDTH / 2; x++) {
                int left_idx = (y * XTION_COLOR_WIDTH + x) * 3;
                int right_idx = (y * XTION_COLOR_WIDTH + (XTION_COLOR_WIDTH - 1 - x)) * 3;
                for (int c = 0; c < 3; c++) {
                    std::swap(data[left_idx + c], data[right_idx + c]);
                }
            }
        }
    }

    return frame;
}

void ColorStream::convertYUV422toRGB888(const uint8_t* yuv, uint8_t* rgb, size_t pixel_count) {
    // YUV422 format: Y0 U Y1 V (4 bytes for 2 pixels)
    for (size_t i = 0; i < pixel_count; i += 2) {
        size_t yuv_idx = (i / 2) * 4;
        size_t rgb_idx = i * 3;

        int y0 = yuv[yuv_idx];
        int u = yuv[yuv_idx + 1];
        int y1 = yuv[yuv_idx + 2];
        int v = yuv[yuv_idx + 3];

        // Convert to RGB
        int c0 = y0 - 16;
        int c1 = y1 - 16;
        int d = u - 128;
        int e = v - 128;

        // First pixel
        int r0 = (298 * c0 + 409 * e + 128) >> 8;
        int g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
        int b0 = (298 * c0 + 516 * d + 128) >> 8;

        rgb[rgb_idx] = std::max(0, std::min(255, r0));
        rgb[rgb_idx + 1] = std::max(0, std::min(255, g0));
        rgb[rgb_idx + 2] = std::max(0, std::min(255, b0));

        // Second pixel
        if (i + 1 < pixel_count) {
            int r1 = (298 * c1 + 409 * e + 128) >> 8;
            int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
            int b1 = (298 * c1 + 516 * d + 128) >> 8;

            rgb[rgb_idx + 3] = std::max(0, std::min(255, r1));
            rgb[rgb_idx + 4] = std::max(0, std::min(255, g1));
            rgb[rgb_idx + 5] = std::max(0, std::min(255, b1));
        }
    }
}

void ColorStream::updateFPS() {
    uint64_t current_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (last_frame_time_ > 0) {
        uint64_t delta = current_time - last_frame_time_;
        if (delta > 0) {
            current_fps_ = 1000000.0f / delta;
        }
    }

    last_frame_time_ = current_time;
}

} // namespace xtion
