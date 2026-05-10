#include "RtmpPublisher.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
}

namespace video {

RtmpPublisher::RtmpPublisher() = default;
RtmpPublisher::~RtmpPublisher() { close(); }

bool RtmpPublisher::init(const std::string& rtmp_url,
                          AVCodecContext* video_codec_ctx,
                          AVCodecContext* audio_codec_ctx) {
    int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "flv", rtmp_url.c_str());
    if (ret < 0 || !fmt_ctx_) {
        std::cerr << "Cannot allocate RTMP output context\n";
        return false;
    }

    fmt_ctx_->max_delay = 100000;

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

    ret = avio_open(&fmt_ctx_->pb, rtmp_url.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot open RTMP URL: " << errbuf << "\n";
        return false;
    }

    ret = avformat_write_header(fmt_ctx_, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot write RTMP header\n";
        return false;
    }

    connected_ = true;
    return true;
}

bool RtmpPublisher::send_packet(AVPacket* pkt) {
    if (!fmt_ctx_ || !connected_) return false;

    AVStream* stream = nullptr;
    if (video_stream_ && pkt->stream_index == video_stream_->index) {
        stream = video_stream_;
    } else if (audio_stream_ && pkt->stream_index == audio_stream_->index) {
        stream = audio_stream_;
    }

    if (!stream) return false;

    pkt->stream_index = stream->index;
    av_packet_rescale_ts(pkt, {1, 1000000}, stream->time_base);

    int ret = av_interleaved_write_frame(fmt_ctx_, pkt);
    return ret >= 0;
}

void RtmpPublisher::close() {
    if (!fmt_ctx_) return;

    if (connected_) {
        av_write_trailer(fmt_ctx_);
    }

    if (fmt_ctx_->pb) {
        avio_closep(&fmt_ctx_->pb);
    }

    avformat_free_context(fmt_ctx_);
    fmt_ctx_ = nullptr;
    connected_ = false;
}

} // namespace video
