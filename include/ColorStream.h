#ifndef COLOR_STREAM_H
#define COLOR_STREAM_H

#include "XtionCommon.h"
#include "USBCommunication.h"
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace xtion {

/**
 * @brief Color stream handler
 *
 * Manages RGB color data acquisition from the device.
 */
class ColorStream {
public:
    explicit ColorStream(std::shared_ptr<USBCommunication> usb);
    ~ColorStream();

    /**
     * @brief Start color stream acquisition
     * @return Status code
     */
    Status start();

    /**
     * @brief Stop color stream acquisition
     */
    void stop();

    /**
     * @brief Check if stream is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return is_running_; }

    /**
     * @brief Read next color frame
     * @param frame Output frame data
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     * @return Status code
     */
    Status readFrame(ColorFrame& frame, int timeout_ms = 1000);

    /**
     * @brief Get current frame rate
     * @return Frames per second
     */
    float getCurrentFPS() const { return current_fps_; }

    /**
     * @brief Set pixel format
     * @param format Desired pixel format (RGB888 or YUV422)
     */
    void setPixelFormat(PixelFormat format);

    /**
     * @brief Enable/disable color mirroring
     * @param enable Mirror flag
     */
    void setMirroring(bool enable) { mirror_enabled_ = enable; }

    /**
     * @brief Enable/disable auto exposure
     * @param enable Auto exposure flag
     */
    void setAutoExposure(bool enable);

    /**
     * @brief Enable/disable auto white balance
     * @param enable Auto white balance flag
     */
    void setAutoWhiteBalance(bool enable);

private:
    /**
     * @brief Stream acquisition thread function
     */
    void acquisitionThread();

    /**
     * @brief Process raw color data
     * @param raw_data Raw data buffer
     * @param size Data size
     * @return Processed color frame
     */
    ColorFrame processRawData(const uint8_t* raw_data, size_t size);

    /**
     * @brief Convert YUV422 to RGB888
     * @param yuv YUV422 data
     * @param rgb Output RGB888 data
     * @param pixel_count Number of pixels
     */
    void convertYUV422toRGB888(const uint8_t* yuv, uint8_t* rgb, size_t pixel_count);

    /**
     * @brief Calculate FPS
     */
    void updateFPS();

    std::shared_ptr<USBCommunication> usb_;
    std::atomic<bool> is_running_;
    std::thread acquisition_thread_;

    // Frame queue
    std::queue<ColorFrame> frame_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    static constexpr size_t MAX_QUEUE_SIZE = 4;

    // Configuration
    PixelFormat pixel_format_;
    bool mirror_enabled_;
    bool auto_exposure_;
    bool auto_white_balance_;

    // Statistics
    float current_fps_;
    uint64_t last_frame_time_;
    uint32_t frame_counter_;

    static constexpr size_t COLOR_BUFFER_SIZE = XTION_COLOR_WIDTH * XTION_COLOR_HEIGHT * 3;
};

} // namespace xtion

#endif // COLOR_STREAM_H
