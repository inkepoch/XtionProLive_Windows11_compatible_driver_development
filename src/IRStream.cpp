#include "IRStream.h"
#include <chrono>
#include <cstring>
#include <iostream>

namespace xtion {

IRStream::IRStream(std::shared_ptr<USBCommunication> usb)
    : usb_(usb)
    , is_running_(false)
    , mirror_enabled_(false)
    , current_fps_(0.0f)
    , last_frame_time_(0)
    , frame_counter_(0) {
}

IRStream::~IRStream() {
    stop();
}

Status IRStream::start() {
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
    acquisition_thread_ = std::thread(&IRStream::acquisitionThread, this);

    return Status::OK;
}

void IRStream::stop() {
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

Status IRStream::readFrame(IRFrame& frame, int timeout_ms) {
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

void IRStream::acquisitionThread() {
    std::vector<uint8_t> buffer(IR_BUFFER_SIZE);

    while (is_running_) {
        size_t received = 0;
        Status status = usb_->bulkRead(EP_IR_IN, buffer.data(), buffer.size(), received, 100);

        if (status == Status::ERROR_TIMEOUT) {
            continue;
        } else if (status != Status::OK) {
            std::cerr << "IR stream read error: " << statusToString(status) << std::endl;
            continue;
        }

        if (received == 0) {
            continue;
        }

        // Process raw data into IR frame
        IRFrame frame = processRawData(buffer.data(), received);

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

IRFrame IRStream::processRawData(const uint8_t* raw_data, size_t size) {
    IRFrame frame;

    // Set metadata
    frame.metadata.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    frame.metadata.frame_index = frame_counter_++;
    frame.metadata.width = XTION_IR_WIDTH;
    frame.metadata.height = XTION_IR_HEIGHT;
    frame.metadata.format = PixelFormat::GRAY16;
    frame.metadata.stride = XTION_IR_WIDTH * sizeof(uint16_t);
    frame.metadata.data_size = XTION_IR_WIDTH * XTION_IR_HEIGHT * sizeof(uint16_t);

    // Allocate data buffer
    frame.data = std::shared_ptr<uint16_t[]>(new uint16_t[XTION_IR_WIDTH * XTION_IR_HEIGHT]);

    // Convert raw data to IR intensity values
    // TODO: Implement actual conversion based on protocol specification
    // For now, simple copy assuming raw data is already in 16-bit format
    size_t pixel_count = std::min(size / 2, static_cast<size_t>(XTION_IR_WIDTH * XTION_IR_HEIGHT));
    std::memcpy(frame.data.get(), raw_data, pixel_count * sizeof(uint16_t));

    // Mirror if enabled
    if (mirror_enabled_) {
        uint16_t* data = frame.data.get();
        for (int y = 0; y < XTION_IR_HEIGHT; y++) {
            for (int x = 0; x < XTION_IR_WIDTH / 2; x++) {
                int left_idx = y * XTION_IR_WIDTH + x;
                int right_idx = y * XTION_IR_WIDTH + (XTION_IR_WIDTH - 1 - x);
                std::swap(data[left_idx], data[right_idx]);
            }
        }
    }

    return frame;
}

void IRStream::updateFPS() {
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
