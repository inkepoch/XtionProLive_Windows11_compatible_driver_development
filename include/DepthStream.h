#ifndef DEPTH_STREAM_H
#define DEPTH_STREAM_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace xtion {

/**
 * @brief Depth stream handler
 *
 * Manages depth data acquisition from the device.
 */
class DepthStream {
public:
    explicit DepthStream(std::shared_ptr<USBCommunication> usb);
    ~DepthStream();

    /**
     * @brief Start depth stream acquisition
     * @return Status code
     */
    Status start();

    /**
     * @brief Stop depth stream acquisition
     */
    void stop();

    /**
     * @brief Check if stream is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return is_running_; }

    /**
     * @brief Read next depth frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     * @return Status code
     */
    Status readFrame(DepthFrame& frame, int timeout_ms = 1000);

    /**
     * @brief Get current frame rate
     * @return Frames per second
     */
    float getCurrentFPS() const { return current_fps_; }

    /**
     * @brief Set depth range
     * @param min_mm Minimum range in millimeters
     * @param max_mm Maximum range in millimeters
     */
    void setRange(uint16_t min_mm, uint16_t max_mm);

    /**
     * @brief Enable/disable depth mirroring
     * @param enable Mirror flag
     */
    void setMirroring(bool enable) { mirror_enabled_ = enable; }

private:
    /**
     * @brief Stream acquisition thread function
     */
    void acquisitionThread();

    /**
     * @brief Process raw depth data
     * @param raw_data Raw data buffer
     * @param size Data size
     * @return Processed depth frame
     */
    DepthFrame processRawData(const uint8_t* raw_data, size_t size);

    /**
     * @brief Apply depth filtering
     * @param frame Depth frame to filter
     */
    void applyFiltering(DepthFrame& frame);

    /**
     * @brief Calculate FPS
     */
    void updateFPS();

    std::shared_ptr<USBCommunication> usb_;
    std::atomic<bool> is_running_;
    std::thread acquisition_thread_;

    // Frame queue
    std::queue<DepthFrame> frame_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    static constexpr size_t MAX_QUEUE_SIZE = 4;

    // Configuration
    uint16_t min_range_mm_;
    uint16_t max_range_mm_;
    bool mirror_enabled_;

    // Statistics
    float current_fps_;
    uint64_t last_frame_time_;
    uint32_t frame_counter_;

    static constexpr size_t DEPTH_BUFFER_SIZE = XTION_DEPTH_WIDTH * XTION_DEPTH_HEIGHT * 2;
};

} // namespace xtion

#endif // DEPTH_STREAM_H
