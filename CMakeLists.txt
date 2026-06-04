cmake_minimum_required(VERSION 3.20)

project(PhotoSync)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(easyexif 
    easyexif/exif.h
    easyexif/exif.cpp
)

target_include_directories(easyexif PUBLIC
    easyexif
)

add_executable(PhotoSync WIN32
    src/console_log_sink.hpp
    src/console_log_sink.cpp
    src/date_parser.hpp
    src/date_parser.cpp
    src/file.hpp
    src/file.cpp
    src/file_manager.hpp
    src/file_manager.cpp
    src/log_init.hpp
    src/log_init.cpp
    src/log_record.hpp
    src/log_sink.hpp
    src/logger.hpp
    src/logger.cpp
    src/main.cpp
    src/photo_sync.hpp
    src/photo_sync.cpp
    src/settings.hpp
    src/settings.cpp
    src/sort_mode.hpp
    src/photo_sync.ui

    resources/PhotoSync.rc
)

target_include_directories(PhotoSync PRIVATE
    C:/SDKs/ffmpeg/include
    src
)

target_link_directories(PhotoSync PRIVATE
    C:/SDKs/ffmpeg/lib
)

target_link_libraries(PhotoSync PRIVATE
    Qt6::Core
    Qt6::Widgets
    easyexif
    avformat
    avcodec
    avutil
)