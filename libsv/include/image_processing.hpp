#pragma once

#include "buffer.hpp"
#include "v4l2_formats.hpp"
#include "platform.hpp"
#include "easylogging++/easylogging++.h"

#include <cstring>

namespace processing {

    inline uint32_t GetLineLength(uint32_t width, uint32_t bpp)
    {
        return (width * bpp) / 8;
    }

    inline uint32_t GetPadding(uint32_t stride, uint32_t lineLength)
    {
        return stride - lineLength;
    }

    inline bool ProcessJetson8BitBuffers(const IImage& input, IProcessedImage& output)
    {
        uint8_t *outputData = (uint8_t *) output.data; 
        uint8_t * const inputData = (uint8_t*)input.data;

        for (int i = 0; i < (int)input.height; ++i) {

            const auto inAdress = i * input.stride;
            const auto outAddress = i * input.width;

            ::memcpy(&outputData[outAddress], &inputData[inAdress], input.width);
        }

        uint8_t *outputEmbeddedData = (uint8_t *) output.embeddedData; 
        uint8_t * const inputEmbeddedData = (uint8_t*)input.embeddedData;
        output.embeddedDataWidth = input.embeddedDataWidth * 2;
        output.embeddedDataHeight = input.embeddedDataHeight;

        for (int i = 0; i < (int)output.embeddedDataHeight; ++i) {

            const auto embeddedDataAdress = i * output.embeddedDataWidth;

            ::memcpy(&outputEmbeddedData[embeddedDataAdress], &inputEmbeddedData[embeddedDataAdress], output.embeddedDataWidth);
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessJetson16BitBuffers(const IImage& input, IProcessedImage& output)
    {
        // with jetson_clocks.sh called
        // only memcpy:                 4.7ms
        // memcpy + shitfing:           5.6ms
        // memcpy + shitfing separated: 7.1ms

        uint8_t *outputData = (uint8_t *) output.data; 
        uint8_t * const inputData = (uint8_t*)input.data;

        const uint32_t outputLineSize = output.width * 2;

        uint16_t mask = 0xFFC0;
        if (v4l2::format::GetBpp(input.pixelFormat) == 12) {
            mask = 0xFFF0;
        }

        for (int i = 0; i < (int)input.height; ++i) {
            
            const auto inAdress = i * input.stride;
            const auto outAddress = i * outputLineSize;

            ::memcpy(&outputData[outAddress], &inputData[inAdress], outputLineSize);

            uint16_t* word = (uint16_t*) &outputData[outAddress];
            for (unsigned int i = 0; i < input.width; i += 1) {
                *word = ((*word) << 2) & mask;
                word += 1;
            }
        }

        uint8_t *outputEmbeddedData = (uint8_t *) output.embeddedData; 
        uint8_t * const inputEmbeddedData = (uint8_t*)input.embeddedData;
        output.embeddedDataWidth = input.embeddedDataWidth * 2;
        output.embeddedDataHeight = input.embeddedDataHeight;

        for (int i = 0; i < (int)output.embeddedDataHeight; ++i) {

            const auto embeddedDataAdress = i * output.embeddedDataWidth;

            ::memcpy(&outputEmbeddedData[embeddedDataAdress], &inputEmbeddedData[embeddedDataAdress], output.embeddedDataWidth);
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessXavier16BitBuffers(const IImage& input, IProcessedImage& output)
    {
        uint8_t *outputData = (uint8_t *) output.data; 
        uint8_t * const inputData = (uint8_t*)input.data;

        const uint32_t outputLineSize = output.width * 2;

        for (int i = 0; i < (int)input.height; ++i) {
            
            const auto inAdress = i * input.stride;
            const auto outAddress = i * outputLineSize;

            ::memcpy(&outputData[outAddress], &inputData[inAdress], outputLineSize);
        }

        uint8_t *outputEmbeddedData = (uint8_t *) output.embeddedData; 
        uint8_t * const inputEmbeddedData = (uint8_t*)input.embeddedData;
        output.embeddedDataWidth = input.embeddedDataWidth * 2;
        output.embeddedDataHeight = input.embeddedDataHeight;

        for (int i = 0; i < (int)output.embeddedDataHeight; ++i) {

            const auto embeddedDataAdress = i * output.embeddedDataWidth;

            ::memcpy(&outputEmbeddedData[embeddedDataAdress], &inputEmbeddedData[embeddedDataAdress], output.embeddedDataWidth);
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessNanoBitBuffers(const IImage& input, IProcessedImage& output)
    {
        uint8_t *outputData = (uint8_t *) output.data; 
        uint8_t * const inputData = (uint8_t*)input.data;

        const uint32_t outputLineSize = output.width * 2;

        uint16_t mask = 0xFFC0;
        uint8_t shift = 6;

        switch(v4l2::format::GetBpp(input.pixelFormat)) {
            case 10:
                mask = 0xFFC0;
                shift = 6;
                break;
            case 12:
                mask = 0xFFF0;
                shift = 4;
                break;
            case 16:
                mask = 0xFFFF;
                shift = 0;
                break;
            default:
                mask = 0xFFC0;
                shift = 6;
        }

        for (int i = 0; i < (int)input.height; ++i) {
            
            const auto inAdress = i * input.stride;
            const auto outAddress = i * outputLineSize;

            ::memcpy(&outputData[outAddress], &inputData[inAdress], outputLineSize);

            uint16_t* word = (uint16_t*) &outputData[outAddress];
            for (unsigned int i = 0; i < input.width; i += 1) {
                *word = ((*word) << shift) & mask;
                word += 1;
            }
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessRaw10Buffers(const IImage& input, IProcessedImage& output)
    {
        uint8_t *inputData = (uint8_t *) input.data;
        uint8_t *outputData = (uint8_t *) output.data; 

        uint64_t outputIndex = 0;
        uint64_t inputIndex = 0;

        uint64_t line, cell;
        uint8_t shift, i;

        uint32_t lineLength = GetLineLength(input.width, 10);
        uint32_t padding = GetPadding(input.stride, lineLength);

        for (line = 0; line < input.height; ++line) {
            for (cell = 0; cell < lineLength; cell += 5) {
                shift = 6;
                for(i = 0; i < 4; ++i) {
                    outputData[outputIndex++] = (inputData[inputIndex + 4] << shift) & 0xC0;
                    outputData[outputIndex++] = inputData[inputIndex + i]; 
                    shift -= 2;
                }
                inputIndex += 5;
            }
            inputIndex += padding;
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessRaw12Buffers(const IImage& input, IProcessedImage& output)
    {
        uint8_t *inputData = (uint8_t *) input.data;
        uint8_t *outputData = (uint8_t *) output.data; 

        uint64_t outputIndex = 0;
        uint64_t inputIndex = 0;

        uint64_t line, cell;

        uint32_t lineLength = GetLineLength(input.width, 12);
        uint32_t padding = GetPadding(input.stride, lineLength);

        for (line = 0; line < input.height; ++line) {
            for (cell = 0; cell < lineLength; cell += 3) {

                outputData[outputIndex++] = inputData[inputIndex + 2] << 4 & 0xF0;
                outputData[outputIndex++] = inputData[inputIndex];

                outputData[outputIndex++] = inputData[inputIndex + 2] & 0xF0;
                outputData[outputIndex++] = inputData[inputIndex + 1];

                inputIndex += 3;
            }
            inputIndex += padding;
        }

        output.timestamp = input.timestamp;

        return true;
    }

    inline bool ProcessJetsonImage(const IImage& input, IProcessedImage& output)
    {
        switch(v4l2::format::GetBpp(input.pixelFormat)) {
            case 8:
                return ProcessJetson8BitBuffers(input, output);
            case 10:
            case 12:
            case 16:
                return ProcessJetson16BitBuffers(input, output);
            default:
                LOG(ERROR) << "Unable to autodetect processing on Jetson, unsupported bit depth " << v4l2::format::GetBpp(input.pixelFormat);
                return false;
        }
    }

    inline bool ProcessXavierImage(const IImage& input, IProcessedImage& output)
    {
        switch(v4l2::format::GetBpp(input.pixelFormat)) {
            case 8:
                return ProcessJetson8BitBuffers(input, output);
            case 10:
            case 12:
            case 16:
                return ProcessXavier16BitBuffers(input, output);
            default:
                LOG(ERROR) << "Unable to autodetect processing on Xavier, unsupported bit depth " << v4l2::format::GetBpp(input.pixelFormat);
                return false;
        }
    }

    inline bool ProcessNanoImage(const IImage& input, IProcessedImage& output)
    {
        switch(v4l2::format::GetBpp(input.pixelFormat)) {
            case 8:
                return ProcessJetson8BitBuffers(input, output);
            case 10:
            case 12:
            case 16:
                return ProcessNanoBitBuffers(input, output);
            default:
                LOG(ERROR) << "Unable to autodetect processing on Nano, unsupported bit depth " << v4l2::format::GetBpp(input.pixelFormat);
                return false;
        }
    }

    inline bool ProcessDragonBoardImage(const IImage& input, IProcessedImage& output)
    {
        switch (v4l2::format::GetBpp(input.pixelFormat)) {
        case 8:
            LOG_N_TIMES(3, INFO) << "No processing has to be performed on 8bit images on DragonBoard.";
            ::memcpy(output.data, input.data, input.length);
            return true;
        case 10:
            return ProcessRaw10Buffers(input, output);
        case 12:
            return ProcessRaw12Buffers(input, output);
        default:
            LOG(ERROR) << "Unable to autodetect processing on DragonBoard, unsupported bit depth " << v4l2::format::GetBpp(input.pixelFormat);
            return false;
        }
    }

    inline bool ProcessAutodetect(const IImage& input, IProcessedImage& output)
    {
        switch (v4l2::platform) {
        case SV_PLATFORM_JETSON_TX2:
        case SV_PLATFORM_JETSON_TX2NX:
            return ProcessJetsonImage(input, output);
        case SV_PLATFORM_DRAGONBOARD_410C:
            return ProcessDragonBoardImage(input, output);
        case SV_PLATFORM_JETSON_XAVIER:
        case SV_PLATFORM_JETSON_XAVIER_NX:
        case SV_PLATFORM_JETSON_AGX_ORIN:
        case SV_PLATFORM_JETSON_AGX_THOR:
        case SV_PLATFORM_JETSON_ORIN_NANO:
        case SV_PLATFORM_JETSON_ORIN_NX:
            return ProcessXavierImage(input, output);
        case SV_PLATFORM_JETSON_NANO:
            return ProcessNanoImage(input, output);
        
        default:
            LOG(ERROR) << "Cannot autodetect processing on an unknown platform";
            return false;
        }
    }
}
