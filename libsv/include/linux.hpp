#pragma once
#include <vector>
#include <string>

namespace linux
{
    std::vector<std::string> GetFilesInDir(std::string directoryName, std::string fileNameRegex);
    int OpenFileDescriptor(const char* file);
    bool CloseFileDescriptor(int fd);
    bool CheckFileDescriptor(const char* file);
    int ReopenFileDescriptor(int fd);
    std::string GetDevnodeName(uint32_t major, uint32_t minor);
    std::string ReadLink(std::string link);
}