/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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
#include "StorageUtils.h"

#include <QByteArray>

#ifndef MMAPPER_NO_ZLIB
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <zlib.h>
#endif

namespace StorageUtils {

#ifndef MMAPPER_NO_ZLIB

template<typename T>
Bytef *as_far_byte_array(T s) = delete;

template<>
inline Bytef *as_far_byte_array(char *const s)
{
    static_assert(alignof(char) == alignof(Bytef));
    return reinterpret_cast<Bytef *>(s);
}

QByteArray inflate(QByteArray &data)
{
    const auto get_zlib_error_str = [](const int ret, const z_stream &strm) {
        std::ostringstream oss;
        oss << "zlib error: (" << ret << ") " << strm.msg;
        return oss.str();
    };

    static constexpr const int CHUNK = 1024;
    char out[CHUNK];
    QByteArray result;

    /* allocate inflate state */
    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = static_cast<uInt>(data.size());
    strm.next_in = as_far_byte_array(data.data());
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        throw std::runtime_error("Unable to initialize zlib");
    }

    /* decompress until deflate stream ends */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = as_far_byte_array(out);
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR); /* state not clobbered */
        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void) inflateEnd(&strm);
            throw std::runtime_error(get_zlib_error_str(ret, strm));
        default:
            break;
        }
        int length = CHUNK - static_cast<int>(strm.avail_out);
        result.append(out, length);

    } while (strm.avail_out == 0);

    /* clean up and return */
    inflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        throw std::runtime_error(get_zlib_error_str(ret, strm));
    }

    return result;
}
#else
QByteArray inflate(QByteArray & /*data*/)
{
    abort();
}
#endif
} // namespace StorageUtils
