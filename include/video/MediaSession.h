#pragma once
#include "Common.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "StreamMuxer.h"
#include "RtmpPublisher.h"
#include <memory>
#include <string>
#include <atomic>
#include <vector>

namespace video {

class MediaSession {
public:
    MediaSession();
    ~MediaSession();

    void configure(const StreamConfig& config);
    bool start();
    void stop();

    bool is_running() const { return running_; }

    struct Stats {
        int64_t video_frames_captured = 0;
        int64_t audio_frames_captured = 0;
        int64_t video_frames_encoded = 0;
        int64_t audio_frames_encoded = 0;
        int64_t bytes_sent = 0;
    };
    Stats get_stats() const;

private:
    void capture_thread_func();
    void encode_thread_func();
    void output_thread_func();

    StreamConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_flag_{false};

    std::unique_ptr<VideoCapture> video_capture_;
    std::unique_ptr<AudioCapture> audio_capture_;

    std::unique_ptr<FrameQueue> video_queue_;
    std::unique_ptr<FrameQueue> audio_queue_;
    std::unique_ptr<PacketQueue> packet_queue_;

    std::unique_ptr<VideoEncoder> video_encoder_;
    std::unique_ptr<AudioEncoder> audio_encoder_;

    std::unique_ptr<StreamMuxer> muxer_;
    std::unique_ptr<RtmpPublisher> publisher_;

    std::vector<std::thread> threads_;

    Stats stats_;
    mutable std::mutex stats_mutex_;
};

} // namespace video
