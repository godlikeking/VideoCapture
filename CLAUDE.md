# CLAUDE.md - 音视频录制与直播项目

## 项目概述

基于 FFmpeg 的 C++ 音视频录制与直播推流程序，使用 CMake 构建系统。

## 技术栈

| 组件 | 选型 |
|------|------|
| 音视频捕获 | FFmpeg libavdevice (DirectShow) |
| 视频编码 | H.264 (libx264) |
| 音频编码 | AAC |
| 封装格式 | MP4 / FLV (RTMP) |
| 构建系统 | CMake 3.16+ |
| 编译器 | MinGW (GCC) 或 MSVC |

## 关键文件

| 文件 | 作用 |
|------|------|
| [CMakeLists.txt](CMakeLists.txt) | 构建配置，包含 FFmpeg 路径 |
| [include/video/MediaSession.h](include/video/MediaSession.h) | 主入口类，协调各模块 |
| [include/video/VideoCapture.h](include/video/VideoCapture.h) | 摄像头捕获，使用 dshow |
| [include/video/AudioCapture.h](include/video/AudioCapture.h) | 麦克风捕获 |
| [include/video/VideoEncoder.h](include/video/VideoEncoder.h) | H.264 视频编码器 |
| [include/video/AudioEncoder.h](include/video/AudioEncoder.h) | AAC 音频编码器 |
| [include/video/RtmpPublisher.h](include/video/RtmpPublisher.h) | RTMP 推流 |
| [src/MediaSession.cpp](src/MediaSession.cpp) | 主逻辑，线程协调 |

## 构建方式

### Windows + MinGW

```powershell
cd f:\Work\Development\code_space\C++\video
mkdir build && cd build
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_MAKE_PROGRAM="C:/Program Files/mingw64/bin/mingw32-make.exe" ^
    -DCMAKE_CXX_COMPILER="C:/Program Files/mingw64/bin/g++.exe"
mingw32-make
```

### FFmpeg 依赖路径

CMakeLists.txt 中配置的 FFmpeg 路径：
- 头文件：`F:/Work/Development/environment/ffmpeg-8.0.1-full_build-shared/include`
- 库文件：`F:/Work/Development/environment/ffmpeg-8.0.1-full_build-shared/lib`
- DLL：`F:/Work/Development/environment/ffmpeg-8.0.1-full_build-shared/bin`

## FFmpeg 8.x API 注意事项

FFmpeg 8.x 有以下 API 变更，修改代码时需注意：

| 旧 API | 新 API |
|--------|--------|
| `FF_PROFILE_H264_HIGH` | `AV_PROFILE_H264_HIGH` |
| `FF_PROFILE_AAC_LOW_COMPLEXITY` | `AV_PROFILE_AAC_LOW` |
| `AVFrame.channels` | `AVFrame.ch_layout.nb_channels` |
| `AVFrame.channel_layout` | `AVFrame.ch_layout` (AVChannelLayout) |
| `AVCodecContext.channels` | `AVCodecContext.ch_layout` |
| `av_find_input_format()` 返回 `const AVInputFormat*` | 需用 `const AVInputFormat*` |
| `av_image_get_buffer_size()` | 需 `#include <libavutil/imgutils.h>` |

## 线程模型

```
捕获线程 (VideoCapture + AudioCapture)
    │
    ▼
FrameQueue (video_queue_, audio_queue_)
    │
    ▼
编码线程 (VideoEncoder + AudioEncoder)
    │
    ▼
PacketQueue (packet_queue_)
    │
    ▼
输出线程 (StreamMuxer → MP4, RtmpPublisher → RTMP)
```

## 设备名称格式

Windows DirectShow 格式：
- 摄像头：`video=0`、`video=Integrated Camera`
- 麦克风：`audio=0`、`audio=Microphone Array`

## 修改 CMakeLists.txt 后

如果修改了 CMakeLists.txt，必须删除缓存后重新配置：

```bash
rm -rf build
mkdir build && cd build
cmake ..
```

## 常见编译错误

1. **找不到 Common.h 等头文件**：确保 `target_include_directories` 包含 `include` 和 `include/video` 目录
2. **FF_PROFILE_xxx 未声明**：改用 `AV_PROFILE_xxx` 常量
3. **`channels` 成员不存在**：改用 `ch_layout.nb_channels`
