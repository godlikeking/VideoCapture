#include "MediaSession.h"
#include <iostream>

namespace video {

MediaSession::MediaSession() = default;
MediaSession::~MediaSession() { stop(); }

void MediaSession::configure(const StreamConfig& config) {
    config_ = config;

    video_capture_ = std::make_unique<VideoCapture>();
    audio_capture_ = std::make_unique<AudioCapture>();

    video_queue_ = std::make_unique<FrameQueue>(30);
    audio_queue_ = std::make_unique<FrameQueue>(30);
    packet_queue_ = std::make_unique<PacketQueue>(30);

    video_encoder_ = std::make_unique<VideoEncoder>();
    audio_encoder_ = std::make_unique<AudioEncoder>();
}

bool MediaSession::start() {
    if (running_) return false;

    if (!video_capture_->init(0, config_.video)) {
        std::cerr << "Failed to initialize video capture\n";
        return false;
    }

    if (!audio_capture_->init(0, config_.audio)) {
        std::cerr << "Failed to initialize audio capture\n";
        return false;
    }

    if (!video_encoder_->init(config_.video)) {
        std::cerr << "Failed to initialize video encoder\n";
        return false;
    }

    if (!audio_encoder_->init(config_.audio)) {
        std::cerr << "Failed to initialize audio encoder\n";
        return false;
    }

    if (config_.enable_recording && !config_.record_path.empty()) {
        muxer_ = std::make_unique<StreamMuxer>();
        if (!muxer_->init_recording(config_.record_path,
                                     video_encoder_->get_codec_context(),
                                     audio_encoder_->get_codec_context())) {
            std::cerr << "Failed to initialize recording\n";
            return false;
        }
    }

    if (config_.enable_streaming && !config_.rtmp_url.empty()) {
        publisher_ = std::make_unique<RtmpPublisher>();
        if (!publisher_->init(config_.rtmp_url,
                               video_encoder_->get_codec_context(),
                               audio_encoder_->get_codec_context())) {
            std::cerr << "Failed to initialize RTMP publisher\n";
            return false;
        }
    }

    video_capture_->start();
    audio_capture_->start();

    running_ = true;
    stop_flag_ = false;

    threads_.emplace_back(&MediaSession::capture_thread_func, this);
    threads_.emplace_back(&MediaSession::encode_thread_func, this);
    threads_.emplace_back(&MediaSession::output_thread_func, this);

    return true;
}

void MediaSession::stop() {
    if (!running_) return;

    stop_flag_ = true;

    if (video_queue_) video_queue_->wake_all();
    if (audio_queue_) audio_queue_->wake_all();
    if (packet_queue_) packet_queue_->wake_all();

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
    threads_.clear();

    video_capture_->stop();
    audio_capture_->stop();

    video_encoder_->close();
    audio_encoder_->close();

    if (muxer_) muxer_->close();
    if (publisher_) publisher_->close();

    running_ = false;
}

MediaSession::Stats MediaSession::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void MediaSession::capture_thread_func() {
    while (!stop_flag_) {
        if (!video_capture_->grab()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        auto frame = video_capture_->retrieve();
        if (frame) {
            video_queue_->push(*frame);
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.video_frames_captured++;
        }

        if (!audio_capture_->grab()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        auto audio_frame = audio_capture_->retrieve();
        if (audio_frame) {
            audio_queue_->push(*audio_frame);
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.audio_frames_captured++;
        }
    }
}

void MediaSession::encode_thread_func() {
    while (!stop_flag_) {
        Frame video_frame, audio_frame;

        bool has_video = video_queue_->try_pop(video_frame);
        bool has_audio = audio_queue_->try_pop(audio_frame);

        if (!has_video && !has_audio) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (has_video) {
            AVPacket* pkt = video_encoder_->encode(video_frame);
            if (pkt) {
                Packet packet;
                packet.type = MediaType::Video;
                packet.pts = pkt->pts;
                packet.dts = pkt->dts;
                packet.data.resize(pkt->size);
                memcpy(packet.data.data(), pkt->data, pkt->size);
                packet.flags = pkt->flags;
                packet_queue_->push(packet);
                av_packet_free(&pkt);
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.video_frames_encoded++;
            }
        }

        if (has_audio) {
            AVPacket* pkt = audio_encoder_->encode(audio_frame);
            if (pkt) {
                Packet packet;
                packet.type = MediaType::Audio;
                packet.pts = pkt->pts;
                packet.dts = pkt->dts;
                packet.data.resize(pkt->size);
                memcpy(packet.data.data(), pkt->data, pkt->size);
                packet.flags = pkt->flags;
                packet_queue_->push(packet);
                av_packet_free(&pkt);
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.audio_frames_encoded++;
            }
        }
    }

    if (video_encoder_) {
        AVPacket* pkt = video_encoder_->flush();
        while (pkt) {
            Packet packet;
            packet.type = MediaType::Video;
            packet.pts = pkt->pts;
            packet.dts = pkt->dts;
            packet.data.resize(pkt->size);
            memcpy(packet.data.data(), pkt->data, pkt->size);
            packet.flags = pkt->flags;
            packet_queue_->push(packet);
            av_packet_free(&pkt);
            pkt = video_encoder_->flush();
        }
    }

    if (audio_encoder_) {
        AVPacket* pkt = audio_encoder_->flush();
        while (pkt) {
            Packet packet;
            packet.type = MediaType::Audio;
            packet.pts = pkt->pts;
            packet.dts = pkt->dts;
            packet.data.resize(pkt->size);
            memcpy(packet.data.data(), pkt->data, pkt->size);
            packet.flags = pkt->flags;
            packet_queue_->push(packet);
            av_packet_free(&pkt);
            pkt = audio_encoder_->flush();
        }
    }

    packet_queue_->wake_all();
}

void MediaSession::output_thread_func() {
    while (!stop_flag_) {
        Packet packet;
        if (!packet_queue_->try_pop(packet)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        AVPacket* pkt = av_packet_alloc();
        if (!pkt) continue;

        pkt->pts = packet.pts;
        pkt->dts = packet.dts;
        pkt->size = packet.data.size();
        pkt->data = static_cast<uint8_t*>(av_malloc(packet.data.size()));
        memcpy(pkt->data, packet.data.data(), packet.data.size());
        pkt->flags = packet.flags;

        if (muxer_ && packet.type == MediaType::Video) {
            muxer_->write_packet(pkt);
        }

        if (publisher_ && packet.type == MediaType::Video) {
            publisher_->send_packet(pkt);
        }

        av_packet_free(&pkt);

        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.bytes_sent += packet.data.size();
    }
}

} // namespace video
