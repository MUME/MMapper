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

#include "WinSock.h"

#include <iostream>

#ifdef __MINGW32__
#include <winsock2.h>
#elif defined(_MSC_VER)
#include <WinSock2.h>
#endif

#ifdef WIN32
#include <mstcpip.h>
#endif

#ifdef WIN32
WinSock::WinSock()
{
    // Load WinSock 2.2 lib
    WSADATA wsd;
    WORD requested = MAKEWORD(2, 2);
    if (WSAStartup(requested, &wsd) != 0) {
        std::cerr << "WSAStartup() failed with error code" << WSAGetLastError();
    }
}

WinSock::~WinSock()
{
    // Unload WinSock
    WSACleanup();
}
#else
WinSock::WinSock() = default;
WinSock::~WinSock() = default;
#endif

bool WinSock::tuneKeepAlive(unsigned int socket,
                            unsigned long maxIdleInMillis,
                            unsigned long intervalInMillis)
{
#ifndef WIN32
    return false;
#else
    struct tcp_keepalive keepAliveVals = {
        true,            // TCP keep-alive on.
        maxIdleInMillis, // Delay in millis after no activity before sending first TCP keep-alive packet
        intervalInMillis, // Delay in millis between sending TCP keep-alive packets.
    };
    DWORD bytesReturned = 0xABAB;
    int ret = WSAIoctl(socket,
                       SIO_KEEPALIVE_VALS,
                       &keepAliveVals,
                       sizeof(keepAliveVals),
                       nullptr,
                       0,
                       &bytesReturned,
                       nullptr,
                       nullptr);
    if (ret != 0) {
        std::cerr << "Could not enable TCP Keep-Alive for with error code" << ret
                  << WSAGetLastError();
        return false;
    }

    // Verify that keepalive option is enabled
    char optVal;
    int optLen = sizeof(optVal);
    ret = getsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optVal, &optLen);
    if (ret != 0) {
        std::cerr << "getsockopt(SO_KEEPALIVE) failed with error code" << ret << WSAGetLastError();
        return false;
    }
    if (optVal != 1) {
        std::cerr << "SO_KEEPALIVE was not enabled" << optVal;
        return false;
    }
    return true;
#endif
}
