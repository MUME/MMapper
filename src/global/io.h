#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RuleOf5.h"
#include "macros.h"

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore>
#include <QtGlobal>

class QFile;

namespace io {

template<size_t N>
class NODISCARD buffer final
{
private:
    static constexpr const size_t SIZE = N;
    static constexpr const size_t ALIGNMENT = 4096;
    static_assert(N >= ALIGNMENT);
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
    static_assert(N <= static_cast<size_t>(std::numeric_limits<int>::max()));
    struct NODISCARD alignas(ALIGNMENT) internal final
    {
        // uninitialized
        alignas(ALIGNMENT) char data[N];
    };
    static_assert(sizeof(internal) == SIZE);
    static_assert(alignof(internal) == ALIGNMENT);

    const std::unique_ptr<internal> m_internal = std::make_unique<internal>();

public:
    buffer() = default;
    ~buffer() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(buffer);
    NODISCARD char *data() { return m_internal->data; }
};

enum class NODISCARD IOResultEnum { SUCCESS, FAILURE, EXCEPTION };

template<size_t N, typename Callback>
NODISCARD IOResultEnum readAllAvailable(QIODevice &dev, buffer<N> &buffer, Callback &&callback)
{
    static constexpr const auto MAX_SIZE = static_cast<qint64>(N);
    char *const data = buffer.data();
    while (dev.bytesAvailable() > 0) {
        const auto got = dev.read(data, MAX_SIZE);
        if (got <= 0) {
            callback(QByteArray::fromRawData(data, 0));
            return IOResultEnum::FAILURE;
        }

        assert(got <= MAX_SIZE);
        const int igot = static_cast<int>(got);
        assert(igot >= 0 && static_cast<decltype(got)>(igot) == got);
        // Warning: This is only a VIEW of the data; it's a bug if the callback
        // extends the lifetime of the QByteArray, because we will replace the
        // contents of data the next time we read into the buffer.
        callback(QByteArray::fromRawData(data, igot));
    }
    callback(QByteArray::fromRawData(data, 0));
    return IOResultEnum::SUCCESS;
}

class NODISCARD IOException : public std::runtime_error
{
public:
    explicit IOException(std::nullptr_t) = delete;
    explicit IOException(const char *s)
        : std::runtime_error(s)
    {}
    explicit IOException(const std::string &s)
        : std::runtime_error(s)
    {}
    ~IOException() override;

    IOException() = delete; // must give a reason!
    DEFAULT_CTORS_AND_ASSIGN_OPS(IOException);

public:
    NODISCARD static IOException withCurrentErrno();
    NODISCARD static IOException withErrorNumber(int error_number);
};

class NODISCARD ErrorNumberMessage final
{
private:
    char buf[1024 - sizeof(const char *) - sizeof(int)]{};
    int error_number = 0;
    const char *str = nullptr;

public:
    ErrorNumberMessage() = default;
    explicit ErrorNumberMessage(int error_number) noexcept;

public:
    explicit operator bool() const { return str != nullptr; }
    explicit operator const char *() const { return str; }

public:
    NODISCARD const char *getErrorMessage() const { return str; }
    NODISCARD int getErrorNumber() const { return error_number; }
};
static_assert(sizeof(ErrorNumberMessage) == 1024);

NODISCARD extern bool fsync(QFile &) noexcept(false);

NODISCARD extern IOResultEnum fsyncNoexcept(QFile &) noexcept;

NODISCARD extern bool tuneKeepAlive(qintptr socketDescriptor,
                                    int maxIdle = 60,
                                    int count = 4,
                                    int interval = 60);

} // namespace io
