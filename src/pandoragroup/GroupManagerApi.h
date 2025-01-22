#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/WeakHandle.h"
#include "mmapper2character.h"

#include <QByteArray>

class Mmapper2Group;

class NODISCARD GroupManagerApi final
{
private:
    Mmapper2Group &m_group;

public:
    explicit GroupManagerApi(Mmapper2Group &group)
        : m_group(group)
    {}
    ~GroupManagerApi() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(GroupManagerApi);

public:
    void kickCharacter(const QString &name);
    void sendGroupTell(const QString &msg);

public:
    void sendScoreLineEvent(const QString &arr);
    void sendPromptLineEvent(const QString &arr);
    void sendEvent(CharacterPositionEnum position);
    void sendEvent(CharacterAffectEnum affect, bool enable);
};
