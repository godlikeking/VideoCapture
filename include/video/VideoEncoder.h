#pragma once
#include "Common.h"
#include <memory>
#include <string>

namespace video {

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    bool init(const VideoConfig& config);
    void close();

    AVPacket* encode(const Frame& frame);
    AVPacket* flush();

    AVCodecContext* get_codec_context() const { return codec_ctx_; }

private:
    AVCodecContext* codec_ctx_ = nullptr;
    const AVCodec* codec_ = nullptr;
    AVFrame* sw_frame_ = nullptr;
    VideoConfig config_;
    int64_t frame_count_ = 0;
};

} // namespace video
