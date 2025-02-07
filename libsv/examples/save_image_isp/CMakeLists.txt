cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Select build type: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -pedantic  -pthread -O2)

find_library(LIB_NVBUFSURFACE nvbufsurface  HINT /usr/lib/aarch64-linux-gnu/nvidia)
find_library(LIB_V4L2 v4l2                  HINT /usr/lib/aarch64-linux-gnu)

add_executable(save_image_isp)

target_include_directories(save_image_isp
                            PUBLIC      "./"
                            PUBLIC      "../"
                            PUBLIC      "/usr/src/jetson_multimedia_api/include"
)

target_sources(save_image_isp
                PRIVATE  ./save_image_isp.cpp
                PRIVATE  ../v4l2isp/v4l2isp.cpp
                PRIVATE  ../v4l2isp/common.cpp
)

target_compile_features(save_image_isp PUBLIC cxx_std_14)

target_link_libraries(save_image_isp
                        -lpthread
                        ${LIB_NVBUFSURFACE}
                        ${LIB_V4L2}
)

install (TARGETS save_image_isp RUNTIME DESTINATION ../bin)
