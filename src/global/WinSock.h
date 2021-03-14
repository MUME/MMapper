#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "macros.h"

class NODISCARD WinSock final
{
public:
    WinSock();
    virtual ~WinSock();

    NODISCARD static bool tuneKeepAlive(unsigned int socket,
                                        unsigned long maxIdleMillis,
                                        unsigned long intervalMillis);
};
