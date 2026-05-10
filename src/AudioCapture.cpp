#include "AudioCapture.h"
#include <iostream>

namespace video {

AudioCapture::AudioCapture() = default;
AudioCapture::~AudioCapture() { stop(); }

bool AudioCapture::init(int device_index, const AudioConfig& config) {
    config_ = config;

    const AVInputFormat* fmt = av_find_input_format("dshow");
    if (!fmt) {
        std::cerr << "Cannot find dshow input format\n";
        return false;
    }

    std::string device_name = "audio=" + std::to_string(device_index);
    AVDictionary* options = nullptr;

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), fmt, &options);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot open audio device: " << errbuf << "\n";
        return false;
    }

    av_dict_free(&options);

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot find stream info\n";
        return false;
    }

    audio_stream_index_ = -1;
    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index_ = i;
            break;
        }
    }

    if (audio_stream_index_ < 0) {
        std::cerr << "Cannot find audio stream\n";
        return false;
    }

    AVCodecParameters* codecpar = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Codec not found\n";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    avcodec_parameters_to_context(codec_ctx_, codecpar);

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot open codec\n";
        return false;
    }

    return true;
}

bool AudioCapture::init(const std::string& device_name, const AudioConfig& config) {
    config_ = config;

    const AVInputFormat* fmt = av_find_input_format("dshow");
    if (!fmt) return false;

    AVDictionary* options = nullptr;

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), fmt, &options);
    if (ret < 0) {
        return false;
    }

    av_dict_free(&options);

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) return false;

    audio_stream_index_ = -1;
    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index_ = i;
            break;
        }
    }

    if (audio_stream_index_ < 0) return false;

    AVCodecParameters* codecpar = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return false;

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    avcodec_parameters_to_context(codec_ctx_, codecpar);

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) return false;

    return true;
}

bool AudioCapture::start() {
    running_ = true;
    return true;
}

void AudioCapture::stop() {
    running_ = false;
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
}

bool AudioCapture::grab() {
    if (!fmt_ctx_ || !codec_ctx_) return false;

    AVPacket* packet = av_packet_alloc();
    if (!packet) return false;

    int ret = av_read_frame(fmt_ctx_, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return false;
    }

    if (packet->stream_index != audio_stream_index_) {
        av_packet_free(&packet);
        return true;
    }

    ret = avcodec_send_packet(codec_ctx_, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        av_packet_free(&packet);
        return false;
    }

    ret = avcodec_receive_frame(codec_ctx_, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        av_packet_free(&packet);
        return false;
    }

    std::lock_guard<std::mutex> lock(frame_mutex_);

    size_t frame_size = av_samples_get_buffer_size(
        nullptr, frame->ch_layout.nb_channels, frame->nb_samples,
        static_cast<AVSampleFormat>(frame->format), 0);

    current_frame_ = std::make_shared<Frame>();
    current_frame_->type = MediaType::Audio;
    current_frame_->pts = frame->pts;
    current_frame_->dts = frame->pkt_dts;
    current_frame_->nb_samples = frame->nb_samples;
    current_frame_->channels = frame->ch_layout.nb_channels;
    current_frame_->sample_format = static_cast<AVSampleFormat>(frame->format);
    current_frame_->sample_rate = frame->sample_rate;
    current_frame_->data.resize(frame_size);
    memcpy(current_frame_->data.data(), frame->data[0], frame_size);

    av_frame_free(&frame);
    av_packet_free(&packet);
    return true;
}

std::shared_ptr<Frame> AudioCapture::retrieve() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return current_frame_;
}

std::vector<std::string> AudioCapture::list_devices() {
    std::vector<std::string> devices;
    avdevice_register_all();
    return devices;
}

} // namespace video
