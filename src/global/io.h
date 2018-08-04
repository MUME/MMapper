#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_IO_H
#define MMAPPER_IO_H

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore>
#include <QtGlobal>

class QFile;

namespace io {

template<size_t N>
struct /* TODO: alignas(4096) */ null_padded_buffer final
{
    static_assert(N >= 4096, "");
    /* TODO: alignas(4096) */
    std::array<char, N + 1u> array{};
};

enum class IOResult { SUCCESS, FAILURE, EXCEPTION };

template<size_t N, typename Callback>
IOResult readAllAvailable(QIODevice &dev, null_padded_buffer<N> &buffer, Callback &&callback)
{
    const auto data = buffer.array.data();
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
        auto byteArray = QByteArray::fromRawData(data, igot);
        callback(byteArray);
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
    explicit ErrorNumberMessage() = default;
    explicit ErrorNumberMessage(int error_number) noexcept;

public:
    explicit operator bool() const { return str != nullptr; }
    explicit operator const char *() const { return str; }

public:
    const char *getErrorMessage() const { return str; }
    int getErrorNumber() const { return error_number; }
};
static_assert(sizeof(ErrorNumberMessage) == 1024, "");

bool fsync(QFile &) noexcept(false);

IOResult fsyncNoexcept(QFile &) noexcept;

} // namespace io
#endif //MMAPPER_IO_H
