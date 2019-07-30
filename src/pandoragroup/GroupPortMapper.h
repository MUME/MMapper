#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"

#include <memory>
#include <QByteArray>

class GroupPortMapper final
{
public:
    struct Pimpl;

private:
    std::unique_ptr<Pimpl> m_pimpl;

public:
    GroupPortMapper();
    ~GroupPortMapper();
    DELETE_CTORS_AND_ASSIGN_OPS(GroupPortMapper);

    QByteArray tryGetExternalIp();
    bool tryAddPortMapping(quint16 port);
    bool tryDeletePortMapping(quint16 port);
};
