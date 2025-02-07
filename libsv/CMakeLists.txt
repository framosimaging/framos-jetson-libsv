cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
message(STATUS "CMake Version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}.${CMAKE_TWEAK_VERSION}")

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Select build type: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

project(libsv CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(ELPP_NO_DEFAULT_LOG_FILE) # Prevent creating default log file whenever display_image is launched.
add_compile_options(-Wall -Wextra -Wpedantic -fPIC -fvisibility=hidden -std=c++11 -O2)

add_library(sv SHARED)

target_include_directories(sv PUBLIC "./include")
target_include_directories(sv PUBLIC "./third-party")

target_sources(sv
                PRIVATE     "./src/buffer_allocator.cpp"
                PRIVATE     "./src/buffer_info.cpp"
                PRIVATE     "./src/camera.cpp"
                PRIVATE     "./src/clock.cpp"
                PRIVATE     "./src/frame_interval.cpp"
                PRIVATE     "./src/frame_size.cpp"
                PRIVATE     "./src/generic_control.cpp"
                PRIVATE     "./src/globals.cpp"
                PRIVATE     "./src/image_format.cpp"
                PRIVATE     "./src/image_info_manager.cpp"
                PRIVATE     "./src/int32_control.cpp"
                PRIVATE     "./src/int64_control.cpp"
                PRIVATE     "./src/linux.cpp"
                PRIVATE     "./src/mediapipeline.cpp"
                PRIVATE     "./src/menu_control.cpp"
                PRIVATE     "./src/platform.cpp"
                PRIVATE     "./src/streamer.cpp"
                PRIVATE     "./src/string_util.cpp"
                PRIVATE     "./src/sv_c.cpp"
                PRIVATE     "./src/sv_cpp.cpp"
                PRIVATE     "./src/v4l2_control_creator.cpp"
                PRIVATE     "./src/v4l2_formats.cpp"
)

target_sources(sv
                PRIVATE     "./third-party/easylogging++/easylogging++.cc")

install(TARGETS sv LIBRARY DESTINATION ../lib)
