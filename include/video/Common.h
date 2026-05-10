#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace video {

enum class MediaType { Video, Audio };

struct VideoConfig {
    int width = 1280;
    int height = 720;
    int fps = 30;
    AVPixelFormat pixel_format = AV_PIX_FMT_YUV420P;
    int bit_rate = 2500000;
    int gop_size = 30;
    int max_b_frames = 2;
};

struct AudioConfig {
    int sample_rate = 44100;
    int channels = 2;
    AVSampleFormat sample_format = AV_SAMPLE_FMT_S16;
    int bit_rate = 128000;
};

struct StreamConfig {
    VideoConfig video;
    AudioConfig audio;
    std::string rtmp_url;
    std::string record_path;
    bool enable_recording = false;
    bool enable_streaming = false;
};

struct Frame {
    MediaType type;
    int64_t pts = 0;
    int64_t dts = 0;
    std::vector<uint8_t> data;

    int width = 0;
    int height = 0;
    AVPixelFormat pixel_format = AV_PIX_FMT_NONE;

    int nb_samples = 0;
    int channels = 0;
    AVSampleFormat sample_format = AV_SAMPLE_FMT_NONE;
    int sample_rate = 0;
};

class FrameQueue {
public:
    explicit FrameQueue(size_t max_size = 30);
    ~FrameQueue();

    void push(Frame frame);
    bool try_push(Frame frame);
    void pop(Frame& frame);
    bool try_pop(Frame& frame);

    size_t size() const;
    void clear();
    void wake_all();
    bool is_shutdown() const { return shutdown_; }

private:
    std::queue<Frame> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    size_t max_size_;
    std::atomic<bool> shutdown_{false};
};

struct Packet {
    MediaType type;
    int64_t pts = 0;
    int64_t dts = 0;
    std::vector<uint8_t> data;
    int flags = 0;
};

class PacketQueue {
public:
    explicit PacketQueue(size_t max_size = 30);
    ~PacketQueue();

    void push(Packet packet);
    bool try_push(Packet packet);
    void pop(Packet& packet);
    bool try_pop(Packet& packet);

    size_t size() const;
    void clear();
    void wake_all();
    bool is_shutdown() const { return shutdown_; }

private:
    std::queue<Packet> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    size_t max_size_;
    std::atomic<bool> shutdown_{false};
};

} // namespace video
