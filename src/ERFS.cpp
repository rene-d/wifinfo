// ERFS.cpp - Embedded Read-only File System (ERFS)
// Copyright (c) 2020 René Devichi. All rights reserved.

//
// ERFS is heavily inspired by Microchip Proprietary File System (MPFS2)
// It is intended to provide a small read-only filesystem for ESP8266.
//
// ERFS Structure:
//     [E][P][F][S]
//     [BYTE Ver Hi][BYTE Ver Lo][WORD Number of Files]
//     [Name Hash 0][Name Hash 1]...[Name Hash N]
//     [File Record 0][File Record 1]...[File Record N]
//     [String 0][String 1]...[String N]
//     [File Data 0][File Data 1]...[File Data N]
//
// Name Hash (2 bytes):
//     hash = 0
//     for each(byte in name)
//         hash += byte
//         hash <<= 1
//
//     Technically this means the hash only includes the
//     final 15 characters of a name.
//
// File Record Structure (16 bytes):
//     [DWORD String Ptr]
//     [DWORD Data Ptr]
//     [DWORD Len]
//     [DWORD Timestamp]
//
//     Pointers are absolute addresses within the ERFS image.
//     Timestamp is the UNIX timestamp
//
// String Structure (1 to 64 bytes):
//     ["path/to/file.ext"][0x00]
//
//      All characteres are allowed, / has no special meaning.
//
// File Data Structure (arbitrary length):
//     [File Data]
//
//
// All values are aligned on 4-byte boundaries, eventually padded with 0x00.
//

#include "ERFS.h"
#include <FSImpl.h>
#include <flash_hal.h>

struct ERFSHeader
{
    uint32_t magic;
    uint8_t ver_h;
    uint8_t ver_l;
    uint16_t num_files;
} __attribute__((packed));

struct FATRecord
{
    uint32_t name_ptr;    // address of filename
    uint32_t data_ptr;    // address of blob
    uint32_t data_length; // length of blob
    uint32_t timestamp;   // posix timestamp in seconds
} __attribute__((packed));

#define NAME_MAX_SIZE 64

// returns the next 4-byte boundary
static inline uint32_t align32(uint32_t n)
{
    if ((n & 3) == 0)
    {
        return n;
    }
    else
    {
        return (n | 3) + 1;
    }
}

class ERFSImpl : public fs::FSImpl
{
public:
    explicit ERFSImpl(uint32_t start, uint32_t size) : start_(start), size_(size), num_files_(0) {}
    virtual ~ERFSImpl() {}
    virtual bool setConfig(const fs::FSConfig &cfg) override { return true; }
    virtual bool begin() override;
    virtual void end() override;
    virtual bool format() override { return false; }
    virtual bool info(fs::FSInfo &info) override;
    virtual bool info64(fs::FSInfo64 &info) override;
    virtual fs::FileImplPtr open(const char *path, fs::OpenMode openMode, fs::AccessMode accessMode) override;
    virtual bool exists(const char *path) override;
    virtual fs::DirImplPtr openDir(const char *path) override;
    virtual bool rename(const char *pathFrom, const char *pathTo) override { return false; }
    virtual bool remove(const char *path) override { return false; }
    virtual bool mkdir(const char *path) override { return false; }
    virtual bool rmdir(const char *path) override { return false; }

public:
    uint16_t num_files() const { return num_files_; }

    // Flash hal wrapper function
    bool hal_read(uint32_t addr, uint32_t size, void *dst)
    {

        if (addr + size > size_)
        {
            return false;
        }

        return flash_hal_read(start_ + addr, size, static_cast<uint8_t *>(dst)) == FLASH_HAL_OK;
    }

private:
    uint32_t start_;     // physical address in flash
    uint32_t size_;      // size in bytes
    uint16_t num_files_; // number of files
};

class ERFSFileImpl : public fs::FileImpl
{
public:
    explicit ERFSFileImpl(ERFSImpl *impl, const char *path);

    virtual ~ERFSFileImpl() {}
    virtual size_t write(const uint8_t *buf, size_t size) override { return 0; }
    virtual size_t read(uint8_t *buf, size_t size) override;
    virtual void flush() override {}
    virtual bool seek(uint32_t pos, fs::SeekMode mode) override;
    virtual size_t position() const override { return pos_; }
    virtual size_t size() const override { return record_.data_length; }
    virtual bool truncate(uint32_t size) override { return false; }
    virtual void close() override {}
    virtual const char *name() const override
    {
        const char *p = strrchr(name_, '/');
        return (p != nullptr) ? (p + 1) : name_;
    };
    virtual const char *fullName() const override { return name_; }
    virtual bool isFile() const override { return impl_ != nullptr; }
    virtual bool isDirectory() const override { return false; }

    virtual time_t getLastWrite() override
    {
        return (time_t)record_.timestamp;
    }

private:
    ERFSImpl *impl_;
    FATRecord record_;
    char name_[NAME_MAX_SIZE];
    uint32_t pos_{0};
};

class ERFSDirImpl : public fs::DirImpl
{
public:
    explicit ERFSDirImpl(ERFSImpl *impl) : impl_(impl), index_(0)
    {
        memset(name_, 0, NAME_MAX_SIZE);
        memset(&record_, 0, sizeof(FATRecord));
    }

    virtual fs::FileImplPtr openFile(fs::OpenMode openMode, fs::AccessMode accessMode) override
    {
        if (is_valid())
        {
            return impl_->open(name_, openMode, accessMode);
        }
        return nullptr;
    }

    virtual const char *fileName() override
    {
        return is_valid() ? name_ : "";
    }

    virtual size_t fileSize() override
    {
        return is_valid() ? record_.data_length : 0;
    }

    virtual time_t fileTime() override
    {
        return is_valid() ? record_.timestamp : 0;
    }

    virtual bool isFile() const override
    {
        return is_valid();
    }

    virtual bool isDirectory() const override { return false; }

    virtual bool next() override
    {
        if (index_ < impl_->num_files())
        {
            uint32_t fat_addr = sizeof(ERFSHeader) + align32(2 * impl_->num_files());

            impl_->hal_read(fat_addr + sizeof(FATRecord) * index_, sizeof(FATRecord), &record_);
            impl_->hal_read(record_.name_ptr, NAME_MAX_SIZE - 1, name_);

            ++index_;
            return true;
        }
        return false;
    }

    virtual bool rewind() override
    {
        index_ = 0;
        return true;
    }

private:
    inline bool is_valid() const
    {
        return (index_ != 0) && (index_ <= impl_->num_files());
    }

private:
    ERFSImpl *impl_;           // ERFS implementation
    FATRecord record_;         // FAT record
    char name_[NAME_MAX_SIZE]; // filename
    uint16_t index_;           // file index between 1 and num_files
};

////

fs::FS ERFS = fs::FS(fs::FSImplPtr(new ERFSImpl(FS_PHYS_ADDR, FS_PHYS_SIZE)));

////

bool ERFSImpl::begin()
{
    ERFSHeader header;

    // try to read the eight bytes header
    if (!hal_read(0, sizeof(ERFSHeader), &header))
    {
        return false;
    }

    if (header.magic != 0x53465245 || header.ver_h != 3 || header.ver_l != 2) // 'ERFS'
    {
        return false;
    }

    num_files_ = header.num_files;

    return true;
}

void ERFSImpl::end()
{
    num_files_ = 0;
}

bool ERFSImpl::info(fs::FSInfo &info)
{
    info.totalBytes = size_;

    if (num_files_ > 0)
    {
        // read the last FAT record, its blob is at the very end of the filesystem
        FATRecord record;
        uint32_t fat_addr = sizeof(ERFSHeader) + align32(2 * num_files_);
        hal_read(fat_addr + sizeof(FATRecord) * (num_files_ - 1), sizeof(FATRecord), &record);
        info.usedBytes = record.data_ptr + record.data_length;
    }
    else
    {
        info.usedBytes = sizeof(ERFSHeader);
    }

    info.blockSize = 0;
    info.pageSize = 0;
    info.maxOpenFiles = num_files_;
    info.maxPathLength = NAME_MAX_SIZE - 1;

    return true;
}

bool ERFSImpl::info64(fs::FSInfo64 &info64)
{
    fs::FSInfo info32;
    info(info32);

    info64.totalBytes = info32.totalBytes;
    info64.usedBytes = info32.usedBytes;
    info64.blockSize = info32.blockSize;
    info64.pageSize = info32.pageSize;
    info64.maxOpenFiles = info32.maxOpenFiles;
    info64.maxPathLength = info32.maxPathLength;

    return true;
}

bool ERFSImpl::exists(const char *path)
{
    ERFSFileImpl test(this, path);
    return test.isFile();
}

fs::DirImplPtr ERFSImpl::openDir(const char *path)
{
    if (strcmp(path, "/") != 0)
    {
        return nullptr;
    }
    return std::make_shared<ERFSDirImpl>(this);
}

fs::FileImplPtr ERFSImpl::open(const char *path, fs::OpenMode openMode, fs::AccessMode accessMode)
{
    if ((openMode != fs::OM_DEFAULT) || (accessMode != fs::AM_READ))
    {
        return nullptr;
    }

    fs::FileImplPtr f = std::make_shared<ERFSFileImpl>(this, path);

    // check if we succeeded to open the file
    if (f->isFile())
    {
        return f;
    }

    return nullptr;
}

ERFSFileImpl::ERFSFileImpl(ERFSImpl *impl, const char *path)
{
    uint16_t name_hash;
    uint16_t hash_cache[8];

    impl_ = nullptr;

    memset(name_, 0, NAME_MAX_SIZE);

    if (path[0] == '/')
    {
        path += 1; // skip the leading '/'
    }

    // Calculate the name hash to speed up searching
    name_hash = 0;
    for (const char *p = path; *p != '\0'; ++p)
    {
        name_hash += (uint8_t)*p;
        name_hash <<= 1;
    }

    uint32_t fat_addr = sizeof(ERFSHeader) + align32(2 * impl->num_files());

    for (uint16_t i = 0; i < impl->num_files(); ++i)
    {
        // Read in hashes, and check remainder on a match.  Store 8 in cache for performance
        if ((i % 8) == 0)
        {
            impl->hal_read(sizeof(ERFSHeader) + i * 2, 8 * 2, &hash_cache);
        }

        // If the hash matches, compare the full filename
        if (name_hash == hash_cache[i % 8])
        {
            impl->hal_read(fat_addr + sizeof(FATRecord) * i, sizeof(FATRecord), &record_);
            impl->hal_read(record_.name_ptr, NAME_MAX_SIZE - 1, name_);

            if (strncmp(name_, path, NAME_MAX_SIZE) == 0)
            {
                // save the ERFSImpl pointer, that indicates we have found the file
                impl_ = impl;
                return;
            }
        }
    }
}

bool ERFSFileImpl::seek(uint32_t pos, fs::SeekMode mode)
{
    if (mode == fs::SeekSet)
    {
        if (pos < record_.data_length)
        {
            pos_ = pos;
            return true;
        }
    }
    else if (mode == fs::SeekCur)
    {
        if (pos + pos_ < record_.data_length)
        {
            pos_ += pos;
            return true;
        }
    }
    else if (mode == fs::SeekEnd)
    {
        if (pos <= record_.data_length)
        {
            pos_ = record_.data_length - pos;
            return true;
        }
    }
    return false;
}

size_t ERFSFileImpl::read(uint8_t *buf, size_t size)
{

    if (impl_ == nullptr || buf == nullptr || size == 0)
    {
        return 0;
    }

    if (pos_ + size > record_.data_length)
    {
        size = record_.data_length - pos_;
    }

    // it's difficult to read from flash, because we have to respect the 4-byte alignment
    // - the source pointer could not be aligned
    // - the target buffer could not be aligned
    // - both should be aligned in the manner to transfer memory from flash to ram
    // the solution is to use an intermediate buffer

    uint32_t addr = record_.data_ptr + pos_;

    if ((addr & 3) == (((uintptr_t)buf) & 3))
    {
        // src and dst have the same word alignment
        impl_->hal_read(addr, size, buf);
    }
    else
    {
        uint32_t remain = size;
        static char buf32[256 + 4];

        while (remain != 0)
        {
            uint32_t align = 4 - (align32(addr) - addr);
            uint32_t chunk = remain > 256 ? 256 : remain;

            impl_->hal_read(addr, chunk, buf32 + align);
            memcpy(buf, buf32 + align, chunk);

            buf += chunk;
            remain -= chunk;
            addr += chunk;
        }
    }

    pos_ += size;
    return size;
}
