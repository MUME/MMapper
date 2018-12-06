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

#include "io.h"

#include <cerrno>
#include <cstring>

#ifndef Q_OS_WIN
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef Q_OS_MAC
#include <fcntl.h>
#endif

#ifdef Q_OS_WIN
#include "WinSock.h"
#endif

namespace io {

ErrorNumberMessage::ErrorNumberMessage(const int error_number) noexcept
    : error_number{error_number}
{
#ifdef Q_OS_WIN
    /* nop */
#elif defined(_GNU_SOURCE)
    /* GNU/Linux version can return a pointer to a static string */
    str = ::strerror_r(error_number, buf, sizeof(buf));
#else
    /* BSD version returns 0 on success */
    if (::strerror_r(error_number, buf, sizeof(buf)) == 0) {
        str = buf;
    }
#endif
}

IOException IOException::withErrorNumber(const int error_number)
{
    if (const auto msg = ErrorNumberMessage{error_number}) {
        return IOException{msg.getErrorMessage()};
    }

    return IOException{"unknown error_number: " + std::to_string(error_number)};
}

IOException IOException::withCurrentErrno()
{
    return withErrorNumber(errno);
}

IOException::~IOException() = default;

bool fsync(QFile &file) noexcept(false)
{
    const int handle = file.handle();
#ifdef Q_OS_WIN
    return false;
#elif defined(Q_OS_MAC)
    if (::fcntl(handle, F_FULLFSYNC) == -1) {
        throw IOException::withCurrentErrno();
    }
#else
    if (::fsync(handle) == -1) {
        throw IOException::withCurrentErrno();
    }
#endif
    return true;
}

IOResult fsyncNoexcept(QFile &file) noexcept
{
    try {
        return fsync(file) ? IOResult::SUCCESS : IOResult::FAILURE;
    } catch (...) {
        return IOResult::EXCEPTION;
    }
}

bool tuneKeepAlive(qintptr socketDescriptor, int maxIdle, int count, int interval)
{
#ifdef Q_OS_WIN
    const unsigned int socket = static_cast<unsigned int>(socketDescriptor);
    const auto maxIdleInMillis = static_cast<unsigned long>(maxIdle * 1000);
    const auto intervalInMillis = static_cast<unsigned long>(interval * 1000);
    return WinSock::tuneKeepAlive(socket, maxIdleInMillis, intervalInMillis);
#else
    const int fd = static_cast<int>(socketDescriptor);

    // Verify that keepalive option is enabled
    int optVal;
    socklen_t optLen = sizeof(optVal);
    if (getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optVal, &optLen) < 0) {
        return false;
    }
    if (optVal != 1) {
        return false;
    }

    // Tune that we wait until 'maxIdle' (default: 60) seconds
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));

    // and then send up to 'count' (default: 4) keepalive packets out, then disconnect if no response
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));

    // Send a keepalive packet out every 'interval' (default: 60) seconds
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    return true;
#endif
}

} // namespace io
