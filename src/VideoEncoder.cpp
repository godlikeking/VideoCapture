#include "VideoEncoder.h"
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

namespace video {

VideoEncoder::VideoEncoder() = default;
VideoEncoder::~VideoEncoder() { close(); }

bool VideoEncoder::init(const VideoConfig& config) {
    config_ = config;

    codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec_) {
        std::cerr << "H.264 codec not found\n";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_) return false;

    codec_ctx_->profile = AV_PROFILE_H264_HIGH;
    codec_ctx_->level = 40;
    codec_ctx_->bit_rate = config_.bit_rate;
    codec_ctx_->width = config_.width;
    codec_ctx_->height = config_.height;
    codec_ctx_->gop_size = config_.gop_size;
    codec_ctx_->max_b_frames = config_.max_b_frames;
    codec_ctx_->pix_fmt = config_.pixel_format;

    codec_ctx_->max_b_frames = config_.max_b_frames;
    codec_ctx_->rc_max_rate = config_.bit_rate;
    codec_ctx_->rc_buffer_size = config_.bit_rate * 2;
    codec_ctx_->time_base = {1, config_.fps};

    codec_ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

    if (avcodec_open2(codec_ctx_, codec_, nullptr) < 0) {
        std::cerr << "Cannot open codec\n";
        return false;
    }

    sw_frame_ = av_frame_alloc();
    if (!sw_frame_) return false;

    sw_frame_->format = config_.pixel_format;
    sw_frame_->width = config_.width;
    sw_frame_->height = config_.height;

    av_image_alloc(sw_frame_->data, sw_frame_->linesize,
                   config_.width, config_.height, config_.pixel_format, 1);

    return true;
}

void VideoEncoder::close() {
    if (sw_frame_) {
        av_freep(&sw_frame_->data[0]);
        av_frame_free(&sw_frame_);
        sw_frame_ = nullptr;
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
}

AVPacket* VideoEncoder::encode(const Frame& frame) {
    if (!codec_ctx_ || !sw_frame_) return nullptr;

    sw_frame_->pts = frame.pts;
    sw_frame_->pkt_dts = frame.dts;

    int ret = avcodec_send_frame(codec_ctx_, sw_frame_);
    if (ret < 0) return nullptr;

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return nullptr;

    ret = avcodec_receive_packet(codec_ctx_, pkt);
    if (ret < 0) {
        av_packet_free(&pkt);
        return nullptr;
    }

    frame_count_++;
    return pkt;
}

AVPacket* VideoEncoder::flush() {
    if (!codec_ctx_) return nullptr;

    avcodec_send_frame(codec_ctx_, nullptr);

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return nullptr;

    int ret = avcodec_receive_packet(codec_ctx_, pkt);
    if (ret < 0) {
        av_packet_free(&pkt);
        return nullptr;
    }

    return pkt;
}

} // namespace video
