#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <QByteArray>

#include "../global/WeakHandle.h"
#include "mmapper2character.h"

class Mmapper2Group;

// This is effectively a weak pointer to a virtual interface without the virtual;
// it basically only exists to avoid giving the parser private access to Parser.
class GroupManagerApi final
{
private:
    WeakHandle<Mmapper2Group> m_group;

public:
    explicit GroupManagerApi(WeakHandle<Mmapper2Group> group)
        : m_group(std::move(group))
    {}

public:
    void kickCharacter(const QByteArray &name) const;
    void sendGroupTell(const QByteArray &msg) const;

public:
    void sendScoreLineEvent(const QByteArray &arr) const;
    void sendPromptLineEvent(const QByteArray &arr) const;
    void sendEvent(CharacterPositionEnum position) const;
    void sendCharacterAffectEvent(CharacterAffectEnum affect, bool enable) const;
};
