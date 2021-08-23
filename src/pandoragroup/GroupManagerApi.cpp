// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GroupManagerApi.h"

#include <exception>
#include <QByteArray>

#include "mmapper2group.h"

void GroupManagerApi::kickCharacter(const QByteArray &name) const
{
    if (name.isEmpty())
        throw std::invalid_argument("name");

    m_group.acceptVisitor([&name](Mmapper2Group &group) { group.kickCharacter(name); });
}

void GroupManagerApi::sendGroupTell(const QByteArray &msg) const
{
    if (msg.isEmpty())
        throw std::invalid_argument("msg");

    m_group.acceptVisitor([&msg](Mmapper2Group &group) { group.sendGroupTell(msg); });
}

void GroupManagerApi::sendScoreLineEvent(const QByteArray &arr) const
{
    m_group.acceptVisitor([&arr](Mmapper2Group &group) { group.parseScoreInformation(arr); });
}

void GroupManagerApi::sendPromptLineEvent(const QByteArray &arr) const
{
    m_group.acceptVisitor([&arr](Mmapper2Group &group) { group.parsePromptInformation(arr); });
}

void GroupManagerApi::sendEvent(const CharacterPositionEnum pos) const
{
    m_group.acceptVisitor([pos](Mmapper2Group &group) { group.updateCharacterPosition(pos); });
}

void GroupManagerApi::sendEvent(const CharacterAffectEnum affect, const bool enable) const
{
    m_group.acceptVisitor(
        [affect, enable](Mmapper2Group &group) { group.updateCharacterAffect(affect, enable); });
}
