// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

#include "zpipe.h"

#include "int_cast.h"
#include "macros.h"

#include <cassert>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <zlib.h>

#include <QDebug>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#endif

#ifdef MMAPPER_NO_ZLIB
NODISCARD int zpipe_deflate(ProgressCounter &pc, IFile &source, IFile &dest, int level)
{
    throw std::runtime_error("unable to deflate (built without zlib)");
}
NODISCARD int zpipe_inflate(ProgressCounter &pc, IFile &source, IFile &dest)
{
    throw std::runtime_error("unable to inflate (built without zlib)");
}
#else

static inline constexpr const size_t PAGE_SIZE = 1u << 12;
static_assert(PAGE_SIZE != 0 && (PAGE_SIZE & (PAGE_SIZE - 1)) == 0,
              "PAGE_SIZE must be a power of two");

#define PAGE_ALIGN alignas(PAGE_SIZE)

static inline constexpr const size_t CHUNK = 1u << 14;
static_assert(CHUNK != 0 && (CHUNK & (CHUNK - 1)) == 0, "CHUNK must be a power of two");
static_assert(CHUNK >= PAGE_SIZE);

namespace mmz {

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int zpipe_deflate(ProgressCounter &pc, IFile &source, IFile &dest, int level)
{
    const auto input_size = source.get_bytes_avail_read();
    pc.increaseTotalStepsBy(2 + input_size);

    int ret = Z_OK;
    int flush = Z_NO_FLUSH;
    unsigned have = 0;
    z_stream strm{};
    PAGE_ALIGN unsigned char in[CHUNK];
    PAGE_ALIGN unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    pc.step();
    if (ret != Z_OK) {
        return ret;
    }

    /* compress until end of file */
    do {
        auto got = strm.avail_in = int_cast::exact::checked_cast<uInt>(source.fread(in, CHUNK));
        if (source.ferror()) {
            (void) deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = source.feof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);   /* no bad return value */
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (dest.fwrite(out, have) != have || dest.ferror()) {
                (void) deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0); /* all input will be used */

        pc.step(got);
        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END); /* stream will be complete */

    /* clean up and return */
    (void) deflateEnd(&strm);
    pc.step();
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int zpipe_inflate(ProgressCounter &pc, IFile &source, IFile &dest)
{
    const auto input_size = source.get_bytes_avail_read();
    pc.increaseTotalStepsBy(2 + input_size);

    int ret = Z_OK;
    unsigned have = 0;
    z_stream strm{};
    PAGE_ALIGN unsigned char in[CHUNK];
    PAGE_ALIGN unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    pc.step();
    if (ret != Z_OK) {
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        auto got = strm.avail_in = int_cast::exact::checked_cast<uInt>(source.fread(in, CHUNK));
        if (source.ferror()) {
            (void) inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0) {
            break;
        }
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR; /* and fall through */
                FALLTHROUGH;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void) inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (dest.fwrite(out, have) != have || dest.ferror()) {
                (void) inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        pc.step(got);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void) inflateEnd(&strm);
    pc.step();
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

#if 0
/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin)) {
            fputs("error reading stdin\n", stderr);
        }
        if (ferror(stdout)) {
            fputs("error writing stdout\n", stderr);
        }
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}
#endif
#endif

IFile::~IFile() = default;

} // namespace mmz
namespace mmqt {

QByteArrayInputStream::QByteArrayInputStream(QByteArray ba)
{
    m_buffer.buffer() = ba;
    m_buffer.open(QIODevice::ReadOnly);
}

QByteArrayInputStream::~QByteArrayInputStream() = default;

size_t QByteArrayInputStream::virt_fread(unsigned char *const buf, const size_t bytes)
{
    using U = uint32_t;
    using I = int32_t;
    const auto avail = static_cast<I>(m_buffer.bytesAvailable());
    const size_t requested = bytes;
    static constexpr size_t lim = static_cast<U>(std::numeric_limits<I>::max());
    if (requested > lim) {
        throw std::runtime_error("read is too large");
    }
    auto ireq = static_cast<I>(requested);
    if (ireq > avail) {
        ireq = avail;
    }
    auto ureq = static_cast<U>(ireq);
    if (ureq <= 0) {
        return 0;
    }
    auto wrote = m_buffer.read(reinterpret_cast<char *>(buf), static_cast<int64_t>(ureq));
    return static_cast<size_t>(wrote);
}
size_t QByteArrayInputStream::virt_fwrite(const unsigned char *const /*buf*/, const size_t /*bytes*/)
{
    throw std::runtime_error("read-only");
}
int QByteArrayInputStream::virt_ferror()
{
    return 0;
}
int QByteArrayInputStream::virt_feof()
{
    return m_buffer.bytesAvailable() == 0;
}
int QByteArrayInputStream::virt_fflush()
{
    return 0;
}
size_t QByteArrayInputStream::virt_get_bytes_avail_read()
{
    return static_cast<size_t>(m_buffer.bytesAvailable());
}

QByteArrayOutputStream::QByteArrayOutputStream()
{
    m_ba.open(QIODevice::WriteOnly);
}

QByteArrayOutputStream::~QByteArrayOutputStream() = default;
size_t QByteArrayOutputStream::virt_fread(unsigned char *const /*buf*/, const size_t /*bytes*/)
{
    throw std::runtime_error("write-only");
}
size_t QByteArrayOutputStream::virt_fwrite(const unsigned char *const buf, const size_t bytes)
{
    const size_t wanted = bytes;
    using U = uint32_t;
    using I = int32_t;
    static constexpr size_t lim = static_cast<U>(std::numeric_limits<I>::max());
    if (wanted > lim) {
        throw std::runtime_error("write is too large");
    }
    m_ba.write(reinterpret_cast<const char *>(buf), static_cast<int64_t>(wanted));
    return wanted;
}
int QByteArrayOutputStream::virt_ferror()
{
    return 0;
}
int QByteArrayOutputStream::virt_feof()
{
    return m_ba.size() == 0;
}
int QByteArrayOutputStream::virt_fflush()
{
    return 0;
}
size_t QByteArrayOutputStream::virt_get_bytes_avail_read()
{
    return 0;
}
} // namespace mmqt
