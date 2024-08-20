#!/bin/bash

THISUSER=`whoami`
if [ "x$THISUSER" != "xroot" ]; then
    echo "This script requires root privilege"
    exit 1
fi

apt update
apt --yes install libgtkglext1 libcanberra-gtk0

CURRENT_DIR=$(dirname $(readlink -f $0))
SW_PACK_DIR=$CURRENT_DIR/..

# Install openCV library
cp -R $SW_PACK_DIR/opencv/lib/* /usr/lib/.
cp -R $SW_PACK_DIR/opencv/include/* /usr/local/include/.
cp -R $SW_PACK_DIR/opencv/share/* /usr/local/share/.


# Install libsv library
cp -R $SW_PACK_DIR/libsv/lib/* /usr/lib/.
cp -R $SW_PACK_DIR/libsv/include/* /usr/local/include/.

# reload libraries
ldconfig

echo "Finish"
