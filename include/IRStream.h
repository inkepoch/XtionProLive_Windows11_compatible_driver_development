#ifndef IR_STREAM_H
#define IR_STREAM_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace xtion {

/**
 * @brief Infrared stream handler
 *
 * Manages IR (infrared) data acquisition from the device.
 */
class IRStream {
public:
    explicit IRStream(std::shared_ptr<USBCommunication> usb);
    ~IRStream();

    /**
     * @brief Start IR stream acquisition
     * @return Status code
     */
    Status start();

    /**
     * @brief Stop IR stream acquisition
     */
    void stop();

    /**
     * @brief Check if stream is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return is_running_; }

    /**
     * @brief Read next IR frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     * @return Status code
     */
    Status readFrame(IRFrame& frame, int timeout_ms = 1000);

    /**
     * @brief Get current frame rate
     * @return Frames per second
     */
    float getCurrentFPS() const { return current_fps_; }

    /**
     * @brief Enable/disable IR mirroring
     * @param enable Mirror flag
     */
    void setMirroring(bool enable) { mirror_enabled_ = enable; }

private:
    /**
     * @brief Stream acquisition thread function
     */
    void acquisitionThread();

    /**
     * @brief Process raw IR data
     * @param raw_data Raw data buffer
     * @param size Data size
     * @return Processed IR frame
     */
    IRFrame processRawData(const uint8_t* raw_data, size_t size);

    /**
     * @brief Calculate FPS
     */
    void updateFPS();

    std::shared_ptr<USBCommunication> usb_;
    std::atomic<bool> is_running_;
    std::thread acquisition_thread_;

    // Frame queue
    std::queue<IRFrame> frame_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    static constexpr size_t MAX_QUEUE_SIZE = 4;

    // Configuration
    bool mirror_enabled_;

    // Statistics
    float current_fps_;
    uint64_t last_frame_time_;
    uint32_t frame_counter_;

    static constexpr size_t IR_BUFFER_SIZE = XTION_IR_WIDTH * XTION_IR_HEIGHT * 2;
};

} // namespace xtion

#endif // IR_STREAM_H
