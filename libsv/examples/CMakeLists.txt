cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
message(STATUS "CMake Version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}.${CMAKE_TWEAK_VERSION}")

project(libsv_examples CXX C)

add_subdirectory(acquire_image)
add_subdirectory(display_image)
add_subdirectory(save_image)
add_subdirectory(acquire_image_isp)
add_subdirectory(display_image_isp)
add_subdirectory(save_image_isp)
