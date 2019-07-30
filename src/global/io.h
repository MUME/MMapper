#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore>
#include <QtGlobal>

#include "RuleOf5.h"

class QFile;

namespace io {

template<size_t N>
struct /* TODO: alignas(4096) */ null_padded_buffer final
{
    static_assert(N >= 4096);
    /* TODO: alignas(4096) */
    std::array<char, N + 1u> array{};
};

enum class IOResult { SUCCESS, FAILURE, EXCEPTION };

template<size_t N, typename Callback>
IOResult readAllAvailable(QIODevice &dev, null_padded_buffer<N> &buffer, Callback &&callback)
{
    char *const data = buffer.array.data();
    const auto maxSize = static_cast<qint64>(buffer.array.size()) - 1;
    while (dev.bytesAvailable() != 0) {
        const auto got = dev.read(data, maxSize);
        if (got < 0) {
            callback(QByteArray::fromRawData(data, 0));
            return IOResult::FAILURE;
        }

        if (got == 0)
            continue;

        assert(got <= maxSize);
        data[got] = '\0';
        const int igot = static_cast<int>(got);
        assert(igot >= 0 && static_cast<decltype(got)>(igot) == got);
        callback(QByteArray::fromRawData(data, igot));
    }
    callback(QByteArray::fromRawData(data, 0));
    return IOResult::SUCCESS;
}

class IOException : public std::runtime_error
{
public:
    explicit IOException(std::nullptr_t) = delete;
    explicit IOException(const char *s)
        : std::runtime_error(s)
    {}
    explicit IOException(const std::string &s)
        : std::runtime_error(s)
    {}
    virtual ~IOException() override;

    IOException() = delete; // must give a reason!
    DEFAULT_CTORS_AND_ASSIGN_OPS(IOException);

public:
    static IOException withCurrentErrno();
    static IOException withErrorNumber(const int error_number);
};

class ErrorNumberMessage final
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
    const char *getErrorMessage() const { return str; }
    int getErrorNumber() const { return error_number; }
};
static_assert(sizeof(ErrorNumberMessage) == 1024);

bool fsync(QFile &) noexcept(false);

IOResult fsyncNoexcept(QFile &) noexcept;

bool tuneKeepAlive(qintptr socketDescriptor, int maxIdle = 60, int count = 4, int interval = 60);

} // namespace io
