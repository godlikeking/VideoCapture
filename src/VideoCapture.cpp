#include "VideoCapture.h"
#include <iostream>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace video {

VideoCapture::VideoCapture() = default;
VideoCapture::~VideoCapture() { stop(); }

bool VideoCapture::init(int device_index, const VideoConfig& config) {
    config_ = config;

    const AVInputFormat* fmt = av_find_input_format("dshow");
    if (!fmt) {
        std::cerr << "Cannot find dshow input format\n";
        return false;
    }

    std::string device_name = "video=" + std::to_string(device_index);
    AVDictionary* options = nullptr;
    char options_str[128];
    snprintf(options_str, sizeof(options_str), "%dx%d", config_.width, config_.height);
    av_dict_set(&options, "video_size", options_str, 0);
    snprintf(options_str, sizeof(options_str), "%d", config_.fps);
    av_dict_set(&options, "framerate", options_str, 0);
    av_dict_set(&options, "probesize", "10000000", 0);
    av_dict_set(&options, "buffer_size", "1000000", 0);

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), fmt, &options);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot open video device: " << errbuf << "\n";
        return false;
    }

    av_dict_free(&options);

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot find stream info\n";
        return false;
    }

    video_stream_index_ = -1;
    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            break;
        }
    }

    if (video_stream_index_ < 0) {
        std::cerr << "Cannot find video stream\n";
        return false;
    }

    AVCodecParameters* codecpar = fmt_ctx_->streams[video_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Codec not found\n";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    ret = avcodec_parameters_to_context(codec_ctx_, codecpar);
    if (ret < 0) return false;

    codec_ctx_->framerate = av_guess_frame_rate(fmt_ctx_, fmt_ctx_->streams[video_stream_index_], nullptr);

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        std::cerr << "Cannot open codec\n";
        return false;
    }

    return true;
}

bool VideoCapture::init(const std::string& device_name, const VideoConfig& config) {
    config_ = config;

    const AVInputFormat* fmt = av_find_input_format("dshow");
    if (!fmt) {
        std::cerr << "Cannot find dshow input format\n";
        return false;
    }

    AVDictionary* options = nullptr;
    char options_str[128];
    snprintf(options_str, sizeof(options_str), "%dx%d", config_.width, config_.height);
    av_dict_set(&options, "video_size", options_str, 0);
    snprintf(options_str, sizeof(options_str), "%d", config_.fps);
    av_dict_set(&options, "framerate", options_str, 0);

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), fmt, &options);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot open video device '" << device_name << "': " << errbuf << "\n";
        return false;
    }

    av_dict_free(&options);

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) return false;

    video_stream_index_ = -1;
    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            break;
        }
    }

    if (video_stream_index_ < 0) return false;

    AVCodecParameters* codecpar = fmt_ctx_->streams[video_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return false;

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) return false;

    avcodec_parameters_to_context(codec_ctx_, codecpar);
    codec_ctx_->framerate = av_guess_frame_rate(fmt_ctx_, fmt_ctx_->streams[video_stream_index_], nullptr);

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) return false;

    return true;
}

bool VideoCapture::start() {
    running_ = true;
    return true;
}

void VideoCapture::stop() {
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

bool VideoCapture::grab() {
    if (!fmt_ctx_ || !codec_ctx_) return false;

    AVPacket* packet = av_packet_alloc();
    if (!packet) return false;

    int ret = av_read_frame(fmt_ctx_, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        return false;
    }

    if (packet->stream_index != video_stream_index_) {
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

    size_t frame_size = av_image_get_buffer_size(
        static_cast<AVPixelFormat>(frame->format),
        frame->width, frame->height, 1);

    current_frame_ = std::make_shared<Frame>();
    current_frame_->type = MediaType::Video;
    current_frame_->pts = frame->pts;
    current_frame_->dts = frame->pkt_dts;
    current_frame_->width = frame->width;
    current_frame_->height = frame->height;
    current_frame_->pixel_format = static_cast<AVPixelFormat>(frame->format);
    current_frame_->data.resize(frame_size);
    av_image_copy_to_buffer(current_frame_->data.data(), frame_size,
                            frame->data, frame->linesize,
                            static_cast<AVPixelFormat>(frame->format),
                            frame->width, frame->height, 1);

    av_frame_free(&frame);
    av_packet_free(&packet);
    return true;
}

std::shared_ptr<Frame> VideoCapture::retrieve() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return current_frame_;
}

std::vector<std::string> VideoCapture::list_devices() {
    std::vector<std::string> devices;

    avdevice_register_all();

    AVFormatContext* ctx = avformat_alloc_context();
    if (!ctx) return devices;

    std::string device_name = "video=";

    // Use dummy device listing
    const AVInputFormat* fmt = av_find_input_format("dshow");
    if (!fmt) return devices;

    // List devices using dshow show_device_list option
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_devices", "true", 0);

    // We can't actually list devices easily with libavdevice without opening
    // So we'll return empty and require user to specify device name
    // In production, would use directshow device enumeration

    av_dict_free(&options);
    avformat_free_context(ctx);

    return devices;
}

} // namespace video
