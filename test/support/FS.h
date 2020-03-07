// module téléinformation client
// rene-d 2020

#pragma once

#include <Arduino.h>

namespace fs
{

class Dir
{
    int index_{0};

public:
    bool next()
    {
        return index_++ < 2;
    }
    String fileName() const
    {
        return index_ == 1 ? "fichier1.txt" : "fichier2.bin";
    }

    size_t fileSize() const
    {
        return index_ == 1 ? 1000 : 2000;
    }
};

// Backwards compatible, <4GB filesystem usage
struct FSInfo
{
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};

class FS
{
public:
    bool begin_called{false};

public:
    bool begin()
    {
        begin_called = true;
        return true;
    }

    Dir openDir(const char *)
    {
        Dir d;
        return d;
    }

    void info(FSInfo &info)
    {
        info.totalBytes = 0x100000;
        info.usedBytes = 123000;
        info.blockSize = 512;
        info.pageSize = 1024;
        info.maxOpenFiles = 32;
        info.maxPathLength = 54;
    }
};
} // namespace fs

using fs::Dir;
//using fs::File;
using fs::FSInfo;
using fs::FS;
