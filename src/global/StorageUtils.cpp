// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "StorageUtils.h"

#ifndef MMAPPER_NO_ZLIB
#include "ConfigConsts.h"
#include "zpipe.h"
#endif

#include <array>
#include <limits>
#include <stdexcept>

#include <QByteArray>

#ifndef MMAPPER_NO_ZLIB
#include <zlib.h>
#endif

namespace StorageUtils::mmqt {
#ifdef MMAPPER_NO_ZLIB
QByteArray inflate(ProgressCounter &pc, QByteArray &data)
{
    throw std::runtime_error("unable to inflate (built without zlib)");
}

QByteArray deflate(ProgressCounter &pc, const QByteArray &data)
{
    throw std::runtime_error("unable to deflate (built without zlib)");
}
#else
QByteArray zlib_inflate(ProgressCounter &pc, const QByteArray &data)
{
    ::mmqt::QByteArrayInputStream is{data};
    ::mmqt::QByteArrayOutputStream os;
    int err = mmz::zpipe_inflate(pc, is, os);
    if (err != 0) {
        throw std::runtime_error("error while inflating");
    }
    return std::move(os).get();
}
QByteArray zlib_deflate(ProgressCounter &pc, const QByteArray &data, const int level)
{
    ::mmqt::QByteArrayInputStream is{data};
    ::mmqt::QByteArrayOutputStream os;
    int err = mmz::zpipe_deflate(pc, is, os, level);
    if (err != 0) {
        throw std::runtime_error("error while deflating");
    }
    return std::move(os).get();
}
#endif

QByteArray uncompress(ProgressCounter &pc, const QByteArray &input)
{
    if constexpr (NO_ZLIB) {
        return ::qUncompress(input);
    } else {
        using U = uint32_t;
        using I = int32_t;

        std::array<char, 4> buf; // NOLINT (uninitialized; will be overwritten by memcpy)
        static_assert(sizeof(buf) == 4);
        memcpy(buf.data(), input.data(), 4);

        uint32_t expect = Size::decode(buf);
        if (expect > static_cast<U>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("data is too large to compress");
        }

        QByteArray result = mmqt::zlib_inflate(pc, input.mid(4));
        if (result.size() != static_cast<I>(expect)) {
            throw std::runtime_error("failed to uncompress");
        }
        return result;
    }
}

QByteArray compress(ProgressCounter &pc, const QByteArray &input)
{
    if constexpr (NO_ZLIB) {
        return ::qCompress(input);
    } else {
        using U = uint32_t;
        U size = static_cast<U>(input.size());
        if (size > static_cast<U>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("data is too large to compress");
        }
        auto header = Size::encode(size);
        static_assert(header.size() == 4);
        QByteArray result;
        result.append(header.data(), 4);
        result.append(mmqt::zlib_deflate(pc, input));

        return result;
    }
}

} // namespace StorageUtils::mmqt
