cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Select build type: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -pedantic -std=c++14 -pthread -O2)

find_library(LIBSV_LIB sv)
find_package(OpenCV 4.5.2  EXACT REQUIRED core highgui imgproc imgcodecs)

add_executable(display_image)

target_include_directories(display_image
                            PUBLIC      "./"
                            PUBLIC      "../"
                            PUBLIC      "../../include"
)

target_sources(display_image
                PRIVATE  ./display_image.cpp
                PRIVATE  ../common_cpp/camera_configurator.cpp
                PRIVATE  ../common_cpp/configured_camera.cpp
                PRIVATE  ../common_cpp/debayering_control.cpp
                PRIVATE  ../common_cpp/display_engine.cpp
                PRIVATE  ../common_cpp/hotkey_action.cpp
                PRIVATE  ../common_cpp/image_pipeline.cpp
                PRIVATE  ../common_cpp/image_processor.cpp
                PRIVATE  ../common_cpp/image_util.cpp
                PRIVATE  ../common_cpp/image_writer.cpp
                PRIVATE  ../common_cpp/resize_control.cpp
                PRIVATE  ../common_cpp/string_util.cpp
)

target_link_libraries(display_image
                        -lpthread
                        ${LIBSV_LIB}
                        ${OpenCV_LIBS}
)

install (TARGETS display_image RUNTIME DESTINATION ../bin)
