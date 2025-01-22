// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GroupManagerApi.h"

#include "mmapper2group.h"

#include <exception>

#include <QByteArray>

void GroupManagerApi::kickCharacter(const QString &name)
{
    if (name.isEmpty()) {
        throw std::invalid_argument("name");
    }

    m_group.kickCharacter(name);
}

void GroupManagerApi::sendGroupTell(const QString &msg)
{
    if (msg.isEmpty()) {
        throw std::invalid_argument("msg");
    }

    m_group.sendGroupTell(msg);
}

void GroupManagerApi::sendScoreLineEvent(const QString &arr)
{
    m_group.parseScoreInformation(arr);
}

void GroupManagerApi::sendPromptLineEvent(const QString &arr)
{
    m_group.parsePromptInformation(arr);
}

void GroupManagerApi::sendEvent(const CharacterPositionEnum pos)
{
    m_group.updateCharacterPosition(pos);
}

void GroupManagerApi::sendEvent(const CharacterAffectEnum affect, const bool enable)
{
    m_group.updateCharacterAffect(affect, enable);
}
