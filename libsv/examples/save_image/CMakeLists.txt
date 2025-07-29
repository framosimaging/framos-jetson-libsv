cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Select build type: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -pedantic  -pthread -O2)

find_library(LIBSV_LIB sv)

add_executable(save_image)
add_executable(save_image_c)

target_include_directories(save_image
                            PUBLIC      "./"
                            PUBLIC      "../"
                            PUBLIC      "../../include"
)

target_include_directories(save_image_c
                            PUBLIC      "./"
                            PUBLIC      "../"
                            PUBLIC      "../../include"
)

target_sources(save_image
                PRIVATE  ./save_image.cpp
)

target_sources(save_image_c
                PRIVATE  ./save_image.c
)

target_compile_features(save_image PUBLIC cxx_std_14)

target_link_libraries(save_image
                        -lpthread
                        ${LIBSV_LIB}
)

target_link_libraries(save_image_c
                        -lpthread
                        ${LIBSV_LIB}
)

install (TARGETS save_image save_image_c RUNTIME DESTINATION ../bin)
