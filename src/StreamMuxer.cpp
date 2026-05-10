#include "StreamMuxer.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

namespace video {

StreamMuxer::StreamMuxer() = default;
StreamMuxer::~StreamMuxer() { close(); }

bool StreamMuxer::init_recording(const std::string& filename,
                                  AVCodecContext* video_codec_ctx,
                                  AVCodecContext* audio_codec_ctx) {
    int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "mp4", filename.c_str());
    if (ret < 0 || !fmt_ctx_) {
        std::cerr << "Cannot allocate output context\n";
        return false;
    }

    if (video_codec_ctx) {
        video_stream_ = avformat_new_stream(fmt_ctx_, nullptr);
        if (!video_stream_) return false;

        ret = avcodec_parameters_from_context(video_stream_->codecpar, video_codec_ctx);
        if (ret < 0) return false;

        video_stream_->time_base = video_codec_ctx->time_base;
    }

    if (audio_codec_ctx) {
        audio_stream_ = avformat_new_stream(fmt_ctx_, nullptr);
        if (!audio_stream_) return false;

        ret = avcodec_parameters_from_context(audio_stream_->codecpar, audio_codec_ctx);
        if (ret < 0) return false;

        audio_stream_->time_base = audio_codec_ctx->time_base;
    }

    if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "Cannot open output file\n";
            return false;
        }
    }

    ret = avformat_write_header(fmt_ctx_, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot write header\n";
        return false;
    }

    is_open_ = true;
    return true;
}

bool StreamMuxer::write_packet(AVPacket* pkt) {
    if (!fmt_ctx_ || !is_open_) return false;

    AVStream* stream = nullptr;
    if (pkt->stream_index == video_stream_->index) {
        stream = video_stream_;
    } else if (pkt->stream_index == audio_stream_->index) {
        stream = audio_stream_;
    }

    if (!stream) return false;

    pkt->stream_index = stream->index;
    av_packet_rescale_ts(pkt, {1, 1000000}, stream->time_base);

    int ret = av_interleaved_write_frame(fmt_ctx_, pkt);
    return ret >= 0;
}

void StreamMuxer::close() {
    if (!fmt_ctx_) return;

    if (is_open_) {
        av_write_trailer(fmt_ctx_);
    }

    if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE) && fmt_ctx_->pb) {
        avio_closep(&fmt_ctx_->pb);
    }

    avformat_free_context(fmt_ctx_);
    fmt_ctx_ = nullptr;
    is_open_ = false;
}

} // namespace video
