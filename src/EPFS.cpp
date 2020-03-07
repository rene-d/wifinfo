// module téléinformation client
// rene-d 2020

#include "EPFS.h"
#include <FSImpl.h>
#include <flash_hal.h>

struct EPFSHeader
{
    uint32_t epfs;
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

class EPFSImpl : public fs::FSImpl
{
public:
    EPFSImpl(uint32_t start, uint32_t size) : start_(start), size_(size), num_files_(0) {}
    virtual ~EPFSImpl() {}
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
    uint32_t start_; // address
    uint32_t size_;  // size in bytes
    uint16_t num_files_;
};

class EPFSFileImpl : public fs::FileImpl
{
public:
    EPFSFileImpl(EPFSImpl *impl, const char *path);

    virtual ~EPFSFileImpl() {}
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
    EPFSImpl *impl_;
    FATRecord record_;
    char name_[NAME_MAX_SIZE];
    uint32_t pos_{0};
};

class EPFSDirImpl : public fs::DirImpl
{
public:
    EPFSDirImpl(EPFSImpl *impl) : impl_(impl), current_(0)
    {
        load_record();
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
        return is_valid() ? name_ : nullptr;
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
        if (current_ < impl_->num_files())
        {
            ++current_;
        }
        if (is_valid())
        {
            load_record();
            return true;
        }
        return false;
    }

    virtual bool rewind() override
    {
        current_ = 0;
        return true;
    }

private:
    inline bool is_valid() const
    {
        return current_ < impl_->num_files();
    }

    void load_record()
    {
        if (is_valid())
        {
            uint32_t fat_addr = sizeof(EPFSHeader) + align32(2 * impl_->num_files());

printf("\033[90mhash \033[0m");
            impl_->hal_read(fat_addr + sizeof(FATRecord) * current_, sizeof(FATRecord), &record_);
printf("\033[90mname \033[0m");
            impl_->hal_read(record_.name_ptr, NAME_MAX_SIZE, name_);
        }
    }

private:
    EPFSImpl *impl_;
    FATRecord record_;
    char name_[NAME_MAX_SIZE];
    uint16_t current_ = 0;
};

////

fs::FS EPFS = fs::FS(fs::FSImplPtr(new EPFSImpl(FS_PHYS_ADDR, FS_PHYS_SIZE)));

////

bool EPFSImpl::begin()
{
    EPFSHeader header;

    // try to read the eight bytes header
    if (!hal_read(0, sizeof(EPFSHeader), &header))
    {
        return false;
    }

    if (header.epfs != 0x53465045 || header.ver_h != 3 || header.ver_l != 2) // 'EPFS'
    {
        return false;
    }

    num_files_ = header.num_files;

    return true;
}

void EPFSImpl::end()
{
    num_files_ = 0;
}

bool EPFSImpl::info(fs::FSInfo &info)
{
    FATRecord record;

    // read the last FAT record, its blob is at the very of the FS
    uint32_t fat_addr = sizeof(EPFSHeader) + align32(2 * num_files());
    hal_read(fat_addr + sizeof(FATRecord) * (num_files_ - 1), sizeof(FATRecord), &record);

    info.totalBytes = size_;
    info.usedBytes = record.data_ptr + record.data_length;
    info.blockSize = 0;
    info.pageSize = 0;
    info.maxOpenFiles = num_files_;
    info.maxPathLength = NAME_MAX_SIZE - 1; // 64 with final \0

    return true;
}

bool EPFSImpl::info64(fs::FSInfo64 &info64)
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

bool EPFSImpl::exists(const char *path)
{
    EPFSFileImpl test(this, path);
    return test.isFile();
}

fs::DirImplPtr EPFSImpl::openDir(const char *path)
{
    if (strcmp(path, "/") != 0)
    {
        return nullptr;
    }
    return std::make_shared<EPFSDirImpl>(this);
}

fs::FileImplPtr EPFSImpl::open(const char *path, fs::OpenMode openMode, fs::AccessMode accessMode)
{
    if ((openMode != fs::OM_DEFAULT) || (accessMode != fs::AM_READ))
    {
        return nullptr;
    }

    fs::FileImplPtr f = std::make_shared<EPFSFileImpl>(this, path);

    // check if we succeeded to open the file
    if (f->isFile())
    {
        return f;
    }

    return nullptr;
}

EPFSFileImpl::EPFSFileImpl(EPFSImpl *impl, const char *path)
{
    uint16_t name_hash;
    uint16_t hash_cache[8];

    impl_ = nullptr;

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

    uint32_t fat_addr = sizeof(EPFSHeader) + align32(2 * impl->num_files());

    for (uint16_t i = 0; i < impl->num_files(); ++i)
    {
        // Read in hashes, and check remainder on a match.  Store 8 in cache for performance
        if ((i % 8) == 0)
        {
                printf("\033[90mhash \033[0m");

            impl->hal_read(sizeof(EPFSHeader) + i * 2, 8 * 2, &hash_cache);
        }

        // If the hash matches, compare the full filename
        if (name_hash == hash_cache[i % 8])
        {
            printf("\033[90mfat  \033[0m");
            impl->hal_read(fat_addr + sizeof(FATRecord) * i, sizeof(FATRecord), &record_);
            printf("\033[90name \033[0m");
            impl->hal_read(record_.name_ptr, NAME_MAX_SIZE, name_);

            if (strncmp(name_, path, NAME_MAX_SIZE) == 0)
            {
                // save the EPFSImpl pointer, that indicates we have found the file
                impl_ = impl;
                return;
            }
        }
    }
}

bool EPFSFileImpl::seek(uint32_t pos, fs::SeekMode mode)
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

size_t EPFSFileImpl::read(uint8_t *buf, size_t size)
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
