#pragma once

#include <stdint.h>

enum ResizeValue { FullHd, Hd, Vga, None };

typedef struct FrameSize {
    uint32_t width;
    uint32_t height;
} FrameSize;

typedef struct ResizeEntry {
    enum ResizeValue resizeValue;
    FrameSize frameSize;
} ResizeEntry;