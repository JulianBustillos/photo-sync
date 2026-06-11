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
    src/rotating_file_sink.hpp
    src/rotating_file_sink.cpp
    src/settings.hpp
    src/settings.cpp
    src/sort_mode.hpp
    src/photo_sync.ui

    resources/PhotoSync.rc
)

target_include_directories(PhotoSync PRIVATE
    src
    $ENV{FFmpeg_DIR}/include
)

target_link_directories(PhotoSync PRIVATE
    $ENV{FFmpeg_DIR}/lib
)

target_link_libraries(PhotoSync PRIVATE
    Qt6::Core
    Qt6::Widgets
    easyexif
    avcodec
    avformat
    avutil
)

install(TARGETS PhotoSync 
    RUNTIME 
    DESTINATION PhotoSync
)

get_target_property(QT_CORE_DLL Qt6::Core IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_CORE_DLL}" DIRECTORY)

find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt
    HINTS "$ENV{QT_BIN_DIR}"
    REQUIRED
)

install(CODE "
    execute_process(
        COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                --release
                --no-translations
                --no-compiler-runtime
                --no-system-d3d-compiler
                --no-opengl-sw
                \"\${CMAKE_INSTALL_PREFIX}/PhotoSync/PhotoSync.exe\"
    )
")

install(CODE [[
    file(REMOVE_RECURSE
        "${CMAKE_INSTALL_PREFIX}/PhotoSync/generic"
        "${CMAKE_INSTALL_PREFIX}/PhotoSync/imageformats"
        "${CMAKE_INSTALL_PREFIX}/PhotoSync/networkinformation"
    )

    file(GLOB DX_FILES
        "${CMAKE_INSTALL_PREFIX}/PhotoSync/*dxcompiler*.dll"
        "${CMAKE_INSTALL_PREFIX}/PhotoSync/*dxil*.dll"
    )

    foreach(f IN LISTS DX_FILES)
        file(REMOVE "${f}")
    endforeach()

    message(STATUS "Extra files cleaned !")
]])

set(FFmpeg_DIR $ENV{FFmpeg_DIR})

file(GLOB FFMPEG_DLLS
    "${FFmpeg_DIR}/bin/avcodec-*.dll"
    "${FFmpeg_DIR}/bin/avformat-*.dll"
    "${FFmpeg_DIR}/bin/avutil-*.dll"
    "${FFmpeg_DIR}/bin/swresample-*.dll"
)

install(FILES ${FFMPEG_DLLS}
    DESTINATION PhotoSync
)

install(CODE [[
    file(ARCHIVE_CREATE
        WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
        OUTPUT "PhotoSync.zip"
        PATHS "PhotoSync"
        FORMAT zip
    )

    message(STATUS "${CMAKE_INSTALL_PREFIX}/PhotoSync.zip created !" )
]])