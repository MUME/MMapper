#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"
#include "progresscounter.h"

#include <cstddef>

#include <QBuffer>
#include <QByteArray>

namespace mmz {

struct IFile;
NODISCARD int zpipe_deflate(ProgressCounter &pc, IFile &source, IFile &dest, int level);
NODISCARD int zpipe_inflate(ProgressCounter &pc, IFile &source, IFile &dest);

struct NODISCARD IFile
{
public:
    IFile() = default;
    IFile(const IFile &) = delete;
    IFile &operator=(const IFile &) = delete;
    virtual ~IFile();

public:
    NODISCARD size_t fread(char *const buf, const size_t bytes)
    {
        return fread(reinterpret_cast<unsigned char *>(buf), bytes);
    }
    NODISCARD size_t fread(unsigned char *const buf, const size_t bytes)
    {
        return virt_fread(buf, bytes);
    }
    NODISCARD size_t fwrite(const char *const buf, const size_t bytes)
    {
        return fwrite(reinterpret_cast<const unsigned char *>(buf), bytes);
    }
    NODISCARD size_t fwrite(const unsigned char *const buf, const size_t bytes)
    {
        return virt_fwrite(buf, bytes);
    }
    NODISCARD int ferror() { return virt_ferror(); }
    NODISCARD int feof() { return virt_feof(); }
    NODISCARD int fflush() { return virt_fflush(); }

    NODISCARD size_t get_bytes_avail_read() { return virt_get_bytes_avail_read(); }

private:
    NODISCARD virtual size_t virt_fread(unsigned char *buf, size_t bytes) = 0;
    NODISCARD virtual size_t virt_fwrite(const unsigned char *buf, size_t bytes) = 0;
    NODISCARD virtual int virt_ferror() = 0;
    NODISCARD virtual int virt_feof() = 0;
    NODISCARD virtual int virt_fflush() = 0;
    NODISCARD virtual size_t virt_get_bytes_avail_read() = 0;
};
} // namespace mmz

namespace mmqt {
class NODISCARD QByteArrayInputStream final : public mmz::IFile
{
private:
    QBuffer m_buffer;

public:
    explicit QByteArrayInputStream(QByteArray ba);
    ~QByteArrayInputStream() final;

private:
    NODISCARD size_t virt_fread(unsigned char *buf, size_t bytes) final;
    NODISCARD size_t virt_fwrite(const unsigned char *buf, size_t bytes) final;
    NODISCARD int virt_ferror() final;
    NODISCARD int virt_feof() final;
    NODISCARD int virt_fflush() final;
    NODISCARD size_t virt_get_bytes_avail_read() final;
};

class NODISCARD QByteArrayOutputStream final : public mmz::IFile
{
private:
    QBuffer m_ba;

public:
    QByteArrayOutputStream();
    ~QByteArrayOutputStream() final;
    NODISCARD QByteArray get() && { return std::move(m_ba).buffer(); }

private:
    NODISCARD size_t virt_fread(unsigned char *buf, size_t bytes) final;
    NODISCARD size_t virt_fwrite(const unsigned char *buf, size_t bytes) final;
    NODISCARD int virt_ferror() final;
    NODISCARD int virt_feof() final;
    NODISCARD int virt_fflush() final;
    NODISCARD size_t virt_get_bytes_avail_read() final;
};

} // namespace mmqt
