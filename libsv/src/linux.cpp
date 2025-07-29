#include "linux.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <regex>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h> 
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include "easylogging++/easylogging++.h"

namespace linux
{
    std::vector<std::string> GetFilesInDir(std::string directoryName, std::string fileNameRegex)
    {
        std::vector<std::string> vec;
        DIR *directory = opendir(directoryName.c_str());
        if(directory != NULL)
        {
            struct dirent *file = readdir(directory);
            while (file != NULL){
                if (fnmatch(fileNameRegex.c_str(), file->d_name, 0) == 0){
                    vec.push_back(directoryName + "/" + file->d_name);
                }
                file = readdir(directory);
            }

            closedir(directory);
        }

        std::sort(std::begin(vec), std::end(vec));

        return vec;
    }

    int OpenFileDescriptor(const char* file)
    {
        return open(file, O_RDWR);
    }

    bool CloseFileDescriptor(int fd)
    {
        return close(fd) == 0;
    }

    bool CheckFileDescriptor(const char* file)
    {
        auto fd = OpenFileDescriptor(file);
        if(fd == -1) {
            return false;
        }

        return CloseFileDescriptor(fd);
    }

    int ReopenFileDescriptor(int fd)
    {
        std::string fdFileLink = "/proc/" + std::to_string(::getpid()) + "/fd/" + std::to_string(fd);
        std::string fdFile = ReadLink(fdFileLink);

        CloseFileDescriptor(fd);
        return OpenFileDescriptor(fdFile.c_str());
    }

    std::string GetDevnodeName(uint32_t major, uint32_t minor)
    {
        std::string fileName = "/sys/dev/char/" + std::to_string(major) + ":" + std::to_string(minor) + "/uevent";
        std::ifstream file(fileName);

        std::string line;
        while(std::getline(file, line)) {
            std::smatch match;
            std::string regexPattern = "DEVNAME=(.+)";
            if (std::regex_search(line, match, std::regex(regexPattern)))
                return "/dev/" + std::string(match[1]);
        }

        LOG(ERROR) << "Unable to find dev name in file " << fileName;
        return "Not found";
    }

    std::string ReadLink(std::string link)
    {
        char buffer[PATH_MAX];
        auto length = ::readlink(link.c_str(), buffer, sizeof(buffer) - 1);
        if (length != -1) {
            buffer[length] = '\0';
            return std::string(buffer);
        }

        LOG(ERROR) << "Unable to read link " << link;
        return "Not found";
    }
}
