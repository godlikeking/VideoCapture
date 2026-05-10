#include "AudioEncoder.h"
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace video {

AudioEncoder::AudioEncoder() = default;
AudioEncoder::~AudioEncoder() { close(); }

bool AudioEncoder::init(const AudioConfig& config) {
    config_ = config;

    codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec_) {
        codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC_LATM);
    }
    if (!codec_) {
        std::cerr << "AAC codec not found\n";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_) return false;

    codec_ctx_->profile = AV_PROFILE_AAC_LOW;
    codec_ctx_->bit_rate = config_.bit_rate;
    codec_ctx_->sample_rate = config_.sample_rate;
    codec_ctx_->ch_layout.nb_channels = config_.channels;
    codec_ctx_->ch_layout.u.mask = AV_CH_LAYOUT_STEREO;

    if (avcodec_open2(codec_ctx_, codec_, nullptr) < 0) {
        std::cerr << "Cannot open codec\n";
        return false;
    }

    return true;
}

void AudioEncoder::close() {
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
}

AVPacket* AudioEncoder::encode(const Frame& frame) {
    if (!codec_ctx_) return nullptr;

    AVFrame* av_frame = av_frame_alloc();
    if (!av_frame) return nullptr;

    av_frame->nb_samples = frame.nb_samples;
    av_frame->format = config_.sample_format;
    av_frame->ch_layout.nb_channels = config_.channels;
    av_frame->ch_layout.u.mask = AV_CH_LAYOUT_STEREO;
    av_frame->pts = frame.pts;

    av_samples_alloc(av_frame->data, nullptr, config_.channels,
                    frame.nb_samples, config_.sample_format, 0);

    if (frame.data.size() > 0) {
        memcpy(av_frame->data[0], frame.data.data(), frame.data.size());
    }

    int ret = avcodec_send_frame(codec_ctx_, av_frame);
    if (ret < 0) {
        av_frame_free(&av_frame);
        return nullptr;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        av_frame_free(&av_frame);
        return nullptr;
    }

    ret = avcodec_receive_packet(codec_ctx_, pkt);
    if (ret < 0) {
        av_packet_free(&pkt);
        av_frame_free(&av_frame);
        return nullptr;
    }

    frame_count_++;
    av_frame_free(&av_frame);
    return pkt;
}

AVPacket* AudioEncoder::flush() {
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
