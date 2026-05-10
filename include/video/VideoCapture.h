#pragma once
#include "Common.h"
#include <memory>
#include <string>
#include <vector>

namespace video {

class VideoCapture {
public:
    VideoCapture();
    ~VideoCapture();

    bool init(int device_index, const VideoConfig& config);
    bool init(const std::string& device_name, const VideoConfig& config);

    bool start();
    void stop();

    bool grab();
    std::shared_ptr<Frame> retrieve();

    static std::vector<std::string> list_devices();

    bool is_running() const { return running_; }

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    int video_stream_index_ = -1;
    VideoConfig config_;
    bool running_ = false;

    std::shared_ptr<Frame> current_frame_;
    std::mutex frame_mutex_;
};

} // namespace video
