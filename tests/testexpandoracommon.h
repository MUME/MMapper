#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../src/map/RoomHandle.h"
#include "../src/map/parseevent.h"
#include "../src/map/room.h"

#include <memory>
#include <optional>

#include <QObject>

struct NODISCARD QtRoomWrapper final
{
    std::shared_ptr<RoomHandle> shared;

    NODISCARD auto getTerrainType() const { return shared->getTerrainType(); }
    NODISCARD decltype(auto) getExit(ExitDirEnum dir) const { return shared->getExit(dir); }
};

Q_DECLARE_METATYPE(QtRoomWrapper);
Q_DECLARE_METATYPE(ParseEvent)
Q_DECLARE_METATYPE(ComparisonResultEnum)

class NODISCARD_QOBJECT TestExpandoraCommon final : public QObject
{
    Q_OBJECT

public:
    TestExpandoraCommon();
    ~TestExpandoraCommon() final;

private:
private Q_SLOTS:
    void roomCompareTest_data();
    void roomCompareTest();
};
