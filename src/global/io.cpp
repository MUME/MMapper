// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "io.h"

#include <cerrno>
#include <cstring>

#ifndef Q_OS_WIN
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#ifdef Q_OS_MAC
#include <fcntl.h>
#endif

#ifdef Q_OS_WIN
#include "WinSock.h"
#endif

namespace io {

ErrorNumberMessage::ErrorNumberMessage(const int error_number) noexcept
    : m_error_number{error_number}
{
#ifdef Q_OS_WIN
    /* nop */
#elif defined(__GLIBC__)
    /* GNU/Linux version can return a pointer to a static string */
    m_str = ::strerror_r(error_number, m_buf, sizeof(m_buf));
#else
    /* XSI-compliant/BSD version version returns 0 on success */
    if (::strerror_r(m_error_number, m_buf, sizeof(m_buf)) == 0) {
        m_str = m_buf;
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

bool fsync(QFile &file) CAN_THROW
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

IOResultEnum fsyncNoexcept(QFile &file) noexcept
{
    try {
        return fsync(file) ? IOResultEnum::SUCCESS : IOResultEnum::FAILURE;
    } catch (...) {
        return IOResultEnum::EXCEPTION;
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
    // Enable TCP keepalive
    const int fd = static_cast<int>(socketDescriptor);
    int optVal = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optVal, sizeof(optVal));
    if (ret == -1) {
        qWarning() << "setsockopt(SO_KEEPALIVE) failed with" << ret << errno;
        return false;
    }

#ifdef Q_OS_MAC
    // Tune that we wait until 'maxIdle' (default: 60) seconds
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &maxIdle, sizeof(maxIdle));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPALIVE) failed with" << ret << errno;
        return false;
    }

    // and then send up to 'count' (default: 4) keepalive packets out, then disconnect if no response
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPCNT) failed with" << ret << errno;
        return false;
    }

    // Send a keepalive packet out every 'interval' (default: 60) seconds
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPINTVL) failed with" << ret << errno;
        return false;
    }
#else
    // Tune that we wait until 'maxIdle' (default: 60) seconds
    ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPIDLE) failed with" << ret << errno;
        return false;
    }

    // and then send up to 'count' (default: 4) keepalive packets out, then disconnect if no response
    ret = setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPCNT) failed with" << ret << errno;
        return false;
    }

    // Send a keepalive packet out every 'interval' (default: 60) seconds
    ret = setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    if (ret < 0) {
        qWarning() << "setsockopt(TCP_KEEPINTVL) failed with" << ret << errno;
        return false;
    }

#endif
    // Verify that the keepalive option is enabled
    optVal = 0;
    socklen_t optLen = sizeof(optVal);
    ret = getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optVal, &optLen);
    if (ret == -1) {
        qWarning() << "getsockopt(SO_KEEPALIVE) failed with" << ret << errno;
        return false;
    }
    if (!optVal) {
        qWarning() << "SO_KEEPALIVE was not enabled" << optVal;
        return false;
    }
    return true;
#endif
}

} // namespace io
