cmake_minimum_required(VERSION 3.16)
project(video_streaming LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# FFmpeg paths - assuming pre-built FFmpeg distribution
set(FFMPEG_ROOT "F:/Work/Development/environment/ffmpeg-8.0.1-full_build-shared" CACHE PATH "FFmpeg installation directory")

# Check if FFmpeg headers exist in libav* subdirectories
if(EXISTS "${FFMPEG_ROOT}/include/libavcodec/avcodec.h" AND
   EXISTS "${FFMPEG_ROOT}/include/libavdevice/avdevice.h" AND
   EXISTS "${FFMPEG_ROOT}/include/libavformat/avformat.h" AND
   EXISTS "${FFMPEG_ROOT}/include/libavutil/avutil.h")
    set(FFmpeg_FOUND TRUE)
    set(FFmpeg_INCLUDE_DIRS "${FFMPEG_ROOT}/include")

    set(FFmpeg_avcodec_lib "${FFMPEG_ROOT}/lib/avcodec.lib")
    set(FFmpeg_avdevice_lib "${FFMPEG_ROOT}/lib/avdevice.lib")
    set(FFmpeg_avformat_lib "${FFMPEG_ROOT}/lib/avformat.lib")
    set(FFmpeg_avutil_lib "${FFMPEG_ROOT}/lib/avutil.lib")
    set(FFmpeg_swscale_lib "${FFMPEG_ROOT}/lib/swscale.lib")
    set(FFmpeg_swresample_lib "${FFMPEG_ROOT}/lib/swresample.lib")

    set(FFmpeg_LIBRARIES
        "${FFmpeg_avcodec_lib}" "${FFmpeg_avdevice_lib}" "${FFmpeg_avformat_lib}"
        "${FFmpeg_avutil_lib}" "${FFmpeg_swscale_lib}" "${FFmpeg_swresample_lib}")

    message(STATUS "FFmpeg found at ${FFMPEG_ROOT}")
    message(STATUS "FFmpeg include dirs: ${FFmpeg_INCLUDE_DIRS}")
    message(STATUS "FFmpeg libraries: ${FFmpeg_LIBRARIES}")
else()
    message(FATAL_ERROR "FFmpeg headers not found in ${FFMPEG_ROOT}/include")
endif()

# Source files
set(VIDEO_SOURCES
    src/Common.cpp
    src/VideoCapture.cpp
    src/AudioCapture.cpp
    src/VideoEncoder.cpp
    src/AudioEncoder.cpp
    src/StreamMuxer.cpp
    src/RtmpPublisher.cpp
    src/MediaSession.cpp
)

add_library(video_core STATIC ${VIDEO_SOURCES})
target_include_directories(video_core PRIVATE
    "${FFMPEG_ROOT}/include"
)
target_include_directories(video_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/video"
)
target_link_libraries(video_core PUBLIC ${FFmpeg_LIBRARIES})

# Example
add_executable(record_stream_example example/record_stream_example.cpp)
target_include_directories(record_stream_example PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${FFMPEG_ROOT}/include"
)
target_link_libraries(record_stream_example PRIVATE video_core)

# Set runtime path for FFmpeg DLLs at runtime - must be after targets
if(WIN32 AND NOT MSVC)
    set_target_properties(video_core PROPERTIES
        BUILD_RPATH "${FFMPEG_ROOT}/bin")
    set_target_properties(record_stream_example PROPERTIES
        BUILD_RPATH "${FFMPEG_ROOT}/bin")
endif()