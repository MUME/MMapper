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

WinSock::WinSock()
{
#ifdef WIN32
    // Load Winsock 2.2 lib
    WSADATA wsd;
    WORD requested = MAKEWORD(2, 2);
    if (WSAStartup(requested, &wsd) != 0) {
        std::cerr << "WSAStartup() failed with error code" << WSAGetLastError();
    }
#endif
}

WinSock::~WinSock()
{
#ifdef WIN32
    WSACleanup();
#endif
}

bool WinSock::tuneKeepAlive(unsigned int socket,
                            unsigned long maxIdleInMillis,
                            unsigned long intervalInMillis)
{
#ifndef WIN32
    return false;
#else
    // Verify that keepalive option is enabled
    char optVal;
    int optLen = sizeof(optVal);
    if (getsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optVal, &optLen) == SOCKET_ERROR) {
        std::cerr << "getsockopt(SO_KEEPALIVE) failed with error code" << WSAGetLastError();
        return false;
    }
    if (optVal != 1) {
        std::cerr << "SO_KEEPALIVE was not enabled yet";
        return false;
    }

    struct tcp_keepalive keepAliveVals = {
        true,            // TCP keep-alive on.
        maxIdleInMillis, // Delay in millis after no activity before sending first TCP keep-alive packet
        intervalInMillis, // Delay in millis between sending TCP keep-alive packets.
    };
    DWORD bytesReturned = 0xABAB;
    if (WSAIoctl(socket,
                 SIO_KEEPALIVE_VALS,
                 &keepAliveVals,
                 sizeof(keepAliveVals),
                 nullptr,
                 0,
                 &bytesReturned,
                 nullptr,
                 nullptr)
        != 0) {
        std::cerr << "Could not enable TCP Keep-Alive for with error code" << WSAGetLastError();
        return false;
    }
    return true;
#endif
}
