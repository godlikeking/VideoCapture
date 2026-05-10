#include "video/MediaSession.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace video;

    std::cout << "Video Recording and Streaming Example\n";
    std::cout << "=====================================\n\n";

    MediaSession session;

    StreamConfig config;
    config.video.width = 1280;
    config.video.height = 720;
    config.video.fps = 30;
    config.video.bit_rate = 2500000;

    config.audio.sample_rate = 44100;
    config.audio.channels = 2;

    config.record_path = "./output.mp4";
    config.enable_recording = true;

    // config.rtmp_url = "rtmp://your-server.com/live/stream_key";
    // config.enable_streaming = true;

    session.configure(config);

    std::cout << "Starting session...\n";
    if (!session.start()) {
        std::cerr << "Failed to start session\n";
        return 1;
    }

    std::cout << "Recording... Press Enter to stop.\n";
    std::cin.get();

    std::cout << "Stopping session...\n";
    session.stop();

    auto stats = session.get_stats();
    std::cout << "\nStatistics:\n";
    std::cout << "  Video frames captured: " << stats.video_frames_captured << "\n";
    std::cout << "  Audio frames captured: " << stats.audio_frames_captured << "\n";
    std::cout << "  Video frames encoded: " << stats.video_frames_encoded << "\n";
    std::cout << "  Audio frames encoded: " << stats.audio_frames_encoded << "\n";
    std::cout << "  Bytes sent: " << stats.bytes_sent << "\n";

    std::cout << "\nDone! Check output.mp4 for the recording.\n";

    return 0;
}
