// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "WinSock.h"

#include "Consts.h"
#include "macros.h"

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

bool WinSock::tuneKeepAlive(MAYBE_UNUSED unsigned int socket,
                            MAYBE_UNUSED unsigned long maxIdleInMillis,
                            MAYBE_UNUSED unsigned long intervalInMillis)
{
#ifndef WIN32
    return false;
#else
    // NOTE: C++ does not require the use of "struct tcp_keepalive" here like C does;
    // TODO: fix this if you have access to a microsoft compiler.
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
    char optVal = char_consts::C_NUL;
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
