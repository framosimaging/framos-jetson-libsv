#pragma once

#include <cinttypes>
#include <vector>
#include "sv/sv.h"

struct Buffer
{
    uint8_t *rawData = nullptr;
    uint32_t rawLength = 0;

    uint32_t index = 0;
    uint32_t sequence = 0;

    struct Timestamp timestamp = {};
};