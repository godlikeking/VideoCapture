#pragma once
#include "Common.h"
#include <memory>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

namespace video {

class RtmpPublisher {
public:
    RtmpPublisher();
    ~RtmpPublisher();

    bool init(const std::string& rtmp_url,
              AVCodecContext* video_codec_ctx,
              AVCodecContext* audio_codec_ctx);

    bool send_packet(AVPacket* pkt);
    void close();

    bool is_connected() const { return connected_; }

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVStream* video_stream_ = nullptr;
    AVStream* audio_stream_ = nullptr;
    bool connected_ = false;
};

} // namespace video
