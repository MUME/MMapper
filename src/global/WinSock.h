#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

class WinSock final
{
public:
    WinSock();
    virtual ~WinSock();

    static bool tuneKeepAlive(unsigned int socket,
                              unsigned long maxIdleMillis,
                              unsigned long intervalMillis);
};
