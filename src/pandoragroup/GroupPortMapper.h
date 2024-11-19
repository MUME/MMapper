#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <memory>

#include <QByteArray>

class NODISCARD GroupPortMapper final
{
public:
    struct Pimpl;

private:
    std::unique_ptr<Pimpl> m_pimpl;

public:
    GroupPortMapper();
    ~GroupPortMapper();
    DELETE_CTORS_AND_ASSIGN_OPS(GroupPortMapper);

    NODISCARD QByteArray tryGetExternalIp();
    NODISCARD bool tryAddPortMapping(uint16_t port);
    NODISCARD bool tryDeletePortMapping(uint16_t port);
};
