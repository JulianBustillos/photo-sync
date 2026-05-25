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
    src/main.cpp
    src/DateParser.hpp
    src/DateParser.cpp
    src/FileData.hpp
    src/FileData.cpp
    src/FileManager.hpp
    src/FileManager.cpp
    src/PhotoSync.hpp
    src/PhotoSync.cpp
    src/Settings.hpp
    src/Settings.cpp
    src/PhotoSync.ui
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