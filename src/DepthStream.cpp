#include "DepthStream.h"
#include <chrono>
#include <cstring>
#include <iostream>

namespace xtion {

DepthStream::DepthStream(std::shared_ptr<USBCommunication> usb)
    : usb_(usb)
    , is_running_(false)
    , min_range_mm_(XTION_MIN_RANGE_MM)
    , max_range_mm_(XTION_MAX_RANGE_MM)
    , mirror_enabled_(false)
    , current_fps_(0.0f)
    , last_frame_time_(0)
    , frame_counter_(0) {
}

DepthStream::~DepthStream() {
    stop();
}

Status DepthStream::start() {
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
    acquisition_thread_ = std::thread(&DepthStream::acquisitionThread, this);

    return Status::OK;
}

void DepthStream::stop() {
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

Status DepthStream::readFrame(DepthFrame& frame, int timeout_ms) {
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

void DepthStream::setRange(uint16_t min_mm, uint16_t max_mm) {
    min_range_mm_ = min_mm;
    max_range_mm_ = max_mm;
}

void DepthStream::acquisitionThread() {
    std::vector<uint8_t> buffer(DEPTH_BUFFER_SIZE);

    while (is_running_) {
        size_t received = 0;
        Status status = usb_->bulkRead(EP_DEPTH_IN, buffer.data(), buffer.size(), received, 100);

        if (status == Status::ERROR_TIMEOUT) {
            continue;
        } else if (status != Status::OK) {
            std::cerr << "Depth stream read error: " << statusToString(status) << std::endl;
            continue;
        }

        if (received == 0) {
            continue;
        }

        // Process raw data into depth frame
        DepthFrame frame = processRawData(buffer.data(), received);

        // Apply filtering
        applyFiltering(frame);

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

DepthFrame DepthStream::processRawData(const uint8_t* raw_data, size_t size) {
    DepthFrame frame;

    // Set metadata
    frame.metadata.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    frame.metadata.frame_index = frame_counter_++;
    frame.metadata.width = XTION_DEPTH_WIDTH;
    frame.metadata.height = XTION_DEPTH_HEIGHT;
    frame.metadata.format = PixelFormat::DEPTH_1MM;
    frame.metadata.stride = XTION_DEPTH_WIDTH * sizeof(uint16_t);
    frame.metadata.data_size = XTION_DEPTH_WIDTH * XTION_DEPTH_HEIGHT * sizeof(uint16_t);

    // Allocate data buffer
    frame.data = std::shared_ptr<uint16_t[]>(new uint16_t[XTION_DEPTH_WIDTH * XTION_DEPTH_HEIGHT]);

    // Convert raw data to depth values
    // TODO: Implement actual conversion based on protocol specification
    // For now, simple copy assuming raw data is already in 16-bit format
    size_t pixel_count = std::min(size / 2, static_cast<size_t>(XTION_DEPTH_WIDTH * XTION_DEPTH_HEIGHT));
    std::memcpy(frame.data.get(), raw_data, pixel_count * sizeof(uint16_t));

    return frame;
}

void DepthStream::applyFiltering(DepthFrame& frame) {
    uint16_t* data = frame.data.get();
    size_t pixel_count = XTION_DEPTH_WIDTH * XTION_DEPTH_HEIGHT;

    for (size_t i = 0; i < pixel_count; i++) {
        uint16_t depth = data[i];

        // Range filtering
        if (depth < min_range_mm_ || depth > max_range_mm_) {
            data[i] = XTION_DEPTH_INVALID;
        }
    }

    // Mirror if enabled
    if (mirror_enabled_) {
        for (int y = 0; y < XTION_DEPTH_HEIGHT; y++) {
            for (int x = 0; x < XTION_DEPTH_WIDTH / 2; x++) {
                int left_idx = y * XTION_DEPTH_WIDTH + x;
                int right_idx = y * XTION_DEPTH_WIDTH + (XTION_DEPTH_WIDTH - 1 - x);
                std::swap(data[left_idx], data[right_idx]);
            }
        }
    }
}

void DepthStream::updateFPS() {
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
