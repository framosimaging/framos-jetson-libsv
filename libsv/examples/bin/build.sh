#!/bin/bash

C_COMPILER=gcc
CPP_COMPILER=g++
WARNING_FLAGS="-Wall -Wextra -pedantic"
C_FLAGS="$WARNING_FLAGS -O2 -Wl,-rpath,\$ORIGIN -pthread"
CPP_FLAGS="$C_FLAGS -std=c++14"
LIB_OPENCV="-L/usr/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -Wl,--disable-new-dtags,-rpath,/usr/lib"
INCLUDE="-I../../include -I../. -L../../lib -lsv"
INCLUDE_ISP="-isystem/usr/src/jetson_multimedia_api/include"
LIB_ISP="-L/usr/lib/aarch64-linux-gnu/tegra -lnvbufsurface -lv4l2"

BASEDIR=$(dirname "$0")
cd "$BASEDIR"
mkdir -p ../build
cd ../build

echo Building display_image cpp example...
$CPP_COMPILER ../../examples/display_image/display_image.cpp ../../examples/common_cpp/*.cpp -o display_image $CPP_FLAGS $LIB_OPENCV $INCLUDE
echo Building save_image cpp example...
$CPP_COMPILER ../../examples/save_image/save_image.cpp -o save_image $CPP_FLAGS $INCLUDE
echo Building acquire_image cpp example...
$CPP_COMPILER ../../examples/acquire_image/acquire_image.cpp -o acquire_image $CPP_FLAGS $INCLUDE
echo Building acquire_image c example...
$C_COMPILER ../../examples/acquire_image/acquire_image.c -o acquire_image_c $C_FLAGS $INCLUDE
echo Building save_image c example...
$C_COMPILER ../../examples/save_image/save_image.c -o save_image_c $C_FLAGS $INCLUDE

echo Building acquire_image_isp cpp example...
$CPP_COMPILER ../acquire_image_isp/acquire_image_isp.cpp ../v4l2isp/*.cpp -I../v4l2isp -o acquire_image_isp $CPP_FLAGS $INCLUDE $LIB_ISP $INCLUDE_ISP
echo Building display_image_isp cpp example...
$CPP_COMPILER ../display_image_isp/display_image_isp.cpp ../v4l2isp/*.cpp -I../v4l2isp -o display_image_isp $CPP_FLAGS $LIB_OPENCV $LIB_ISP $INCLUDE $INCLUDE_ISP
echo Building save_image_isp cpp example...
$CPP_COMPILER ../save_image_isp/save_image_isp.cpp ../v4l2isp/*.cpp -I../v4l2isp -o save_image_isp $CPP_FLAGS $LIB_ISP $INCLUDE $INCLUDE_ISP
