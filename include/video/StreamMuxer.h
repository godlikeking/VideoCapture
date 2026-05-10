#pragma once
#include "Common.h"
#include <memory>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

namespace video {

class StreamMuxer {
public:
    StreamMuxer();
    ~StreamMuxer();

    bool init_recording(const std::string& filename,
                        AVCodecContext* video_codec_ctx,
                        AVCodecContext* audio_codec_ctx);

    bool write_packet(AVPacket* pkt);
    void close();

    bool is_open() const { return is_open_; }

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    AVStream* video_stream_ = nullptr;
    AVStream* audio_stream_ = nullptr;
    bool is_open_ = false;
};

} // namespace video
