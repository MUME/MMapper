#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../map/coordinate.h"
#include "../mapdata/mapdata.h"
#include "../mapfrontend/mapfrontend.h"
#include "abstractmapstorage.h"

#include <cstdint>

#include <QArgument>
#include <QObject>
#include <QString>
#include <QtGlobal>

class InfoMark;
class QDataStream;
class QFile;
class QObject;
class Room;

class NODISCARD_QOBJECT MapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MapStorage(MapData &, const QString &, QFile *, QObject *parent);
    explicit MapStorage(MapData &, const QString &, QObject *parent);
    bool mergeData() override;

public:
    NODISCARD bool canLoad() const override { return true; }
    NODISCARD bool canSave() const override { return true; }

private:
    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;

    SharedRoom loadRoom(QDataStream &stream, uint32_t version);
    void loadExits(Room &room, QDataStream &stream, uint32_t version);
    void loadMark(InfoMark &mark, QDataStream &stream, uint32_t version);
    void saveMark(const InfoMark &mark, QDataStream &stream);
    void saveRoom(const Room &room, QDataStream &stream);
    void saveExits(const Room &room, QDataStream &stream);
    void log(const QString &msg) { emit sig_log("MapStorage", msg); }

    uint32_t baseId = 0u;
    Coordinate basePosition;
};

class MapFrontendBlocker final
{
public:
    explicit MapFrontendBlocker(MapFrontend &in_data)
        : data(in_data)
    {
        data.block();
    }
    ~MapFrontendBlocker() { data.unblock(); }

public:
    MapFrontendBlocker() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(MapFrontendBlocker);

private:
    MapFrontend &data;
};
