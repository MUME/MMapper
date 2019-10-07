// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GroupManagerApi.h"

#include <exception>
#include <QByteArray>

#include "mmapper2group.h"

void GroupManagerApi::kickCharacter(const QByteArray &name)
{
    if (name.isEmpty())
        throw std::invalid_argument("name");

    m_group.acceptVisitor([&name](Mmapper2Group &group) { group.kickCharacter(name); });
}

void GroupManagerApi::sendGroupTell(const QByteArray &msg)
{
    if (msg.isEmpty())
        throw std::invalid_argument("msg");

    m_group.acceptVisitor([&msg](Mmapper2Group &group) { group.sendGroupTell(msg); });
}
