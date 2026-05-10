# Video Streaming - 音视频录制与直播程序

基于 FFmpeg 的 C++ 音视频录制与直播推流工具，支持本地录制和 RTMP 直播推流。

## 功能特性

- **摄像头捕获**：使用 DirectShow (Windows) / V4L2 (Linux) 捕获摄像头视频
- **麦克风捕获**：捕获系统音频输入
- **本地录制**：编码并保存为 MP4 文件
- **RTMP 直播推流**：支持推送到主流流媒体服务器

## 技术架构

| 组件 | 技术选型 |
|------|----------|
| 音视频捕获 | FFmpeg libavdevice |
| 视频编码 | H.264 (libx264) |
| 音频编码 | AAC |
| 封装格式 | MP4 |
| 直播协议 | RTMP |
| 构建系统 | CMake 3.16+ |

## 项目结构

```
video-streaming/
├── CMakeLists.txt              # CMake 构建配置
├── include/video/
│   ├── Common.h               # 共享类型定义
│   ├── VideoCapture.h         # 摄像头捕获
│   ├── AudioCapture.h         # 麦克风捕获
│   ├── VideoEncoder.h         # H.264 视频编码器
│   ├── AudioEncoder.h         # AAC 音频编码器
│   ├── StreamMuxer.h          # MP4 封装写入
│   ├── RtmpPublisher.h        # RTMP 推流
│   └── MediaSession.h        # 主会话协调类
├── src/                       # 实现源码
├── example/
│   └── record_stream_example.cpp  # 使用示例
└── README.md
```

## 编译构建

### 环境要求

- **编译器**：MinGW (GCC 9+) 或 MSVC (VS2019+)
- **FFmpeg**：6.x 或 7.x 开发库
- **CMake**：3.16+

### Windows + MinGW 编译步骤

1. 安装 FFmpeg 开发库（预编译版），记录安装路径，例如：
   ```
   F:\Work\Development\environment\ffmpeg-8.0.1-full_build-shared
   ```

2. 配置并编译：
   ```powershell
   cd f:\Work\Development\code_space\C++\video
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles" ^
       -DCMAKE_MAKE_PROGRAM="C:/Program Files/mingw64/bin/mingw32-make.exe" ^
       -DCMAKE_CXX_COMPILER="C:/Program Files/mingw64/bin/g++.exe"
   mingw32-make
   ```

3. 运行：
   ```bash
   cd build
   ./record_stream_example.exe
   ```

### Linux 编译步骤

```bash
sudo apt install libavcodec-dev libavdevice-dev libavformat-dev \
                libavutil-dev libswscale-dev libswresample-dev
mkdir build && cd build
cmake .. && make
./record_stream_example
```

## 使用示例

```cpp
#include "video/MediaSession.h"

int main() {
    using namespace video;

    MediaSession session;

    StreamConfig config;
    config.video.width = 1280;
    config.video.height = 720;
    config.video.fps = 30;
    config.video.bit_rate = 2500000;

    config.audio.sample_rate = 44100;
    config.audio.channels = 2;

    // 本地录制配置
    config.record_path = "./output.mp4";
    config.enable_recording = true;

    // RTMP 推流配置（可选）
    config.rtmp_url = "rtmp://your-server.com/live/stream_key";
    config.enable_streaming = false;

    session.configure(config);

    if (!session.start()) {
        std::cerr << "启动失败\n";
        return 1;
    }

    std::cout << "按 Enter 键停止...\n";
    std::cin.get();

    session.stop();

    auto stats = session.get_stats();
    std::cout << "视频帧: " << stats.video_frames_encoded << "\n"
              << "音频帧: " << stats.audio_frames_encoded << "\n";

    return 0;
}
```

## 线程模型

```
捕获线程 ──▶ 编码线程 ──┬──▶ 录制线程 (MP4)
                       └──▶ 推流线程 (RTMP)
```

- **捕获线程**：从摄像头/麦克风读取原始帧
- **编码线程**：编码后写入输出队列
- **输出线程**：写入文件或推送到流媒体服务器

## 依赖说明

项目依赖以下 FFmpeg 组件库：

- `libavcodec` - 编解码
- `libavdevice` - 设备捕获
- `libavformat` - 封装/解封装
- `libavutil` - 工具库
- `libswscale` - 图像缩放
- `libswresample` - 音频重采样

## 常见问题

**Q: 找不到 FFmpeg 库？**
A: 确保 FFmpeg 开发库已正确安装，并在 CMakeLists.txt 中配置正确的 `FFMPEG_ROOT` 路径。

**Q: 摄像头/麦克风无法打开？**
A: 检查设备名称是否正确。Windows 上使用 DirectShow，设备名格式为 `video=0` 或 `audio=0`。

**Q: 编译报错 `FF_PROFILE_xxx` 未声明？**
A: 这是 FFmpeg 8.x API 变更，请使用 `AV_PROFILE_H264_HIGH` 和 `AV_PROFILE_AAC_LOW`。

## 许可证

MIT License
