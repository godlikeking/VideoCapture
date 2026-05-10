#pragma once
#include "Common.h"
#include <memory>

namespace video {

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();

    bool init(const AudioConfig& config);
    void close();

    AVPacket* encode(const Frame& frame);
    AVPacket* flush();

    AVCodecContext* get_codec_context() const { return codec_ctx_; }

private:
    AVCodecContext* codec_ctx_ = nullptr;
    const AVCodec* codec_ = nullptr;
    SwrContext* swr_ctx_ = nullptr;
    AudioConfig config_;
    int64_t frame_count_ = 0;
};

} // namespace video
