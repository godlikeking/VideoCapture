#pragma once
#include "Common.h"
#include <memory>
#include <string>
#include <vector>

namespace video {

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    bool init(int device_index, const AudioConfig& config);
    bool init(const std::string& device_name, const AudioConfig& config);

    bool start();
    void stop();

    bool grab();
    std::shared_ptr<Frame> retrieve();

    static std::vector<std::string> list_devices();

    bool is_running() const { return running_; }

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    int audio_stream_index_ = -1;
    AudioConfig config_;
    bool running_ = false;

    std::shared_ptr<Frame> current_frame_;
    std::mutex frame_mutex_;
};

} // namespace video
