#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QtCore>

#include "abstractmapstorage.h"

class MapData;
class QJsonArray;
class QJsonObject;
class QObject;

/*! \brief JSON export for web clients.
 *
 * This saves to a directory the following files:
 * - v1/arda.json (global metadata like map size).
 * - v1/roomindex/ss.json (room sums -> zone coords).
 * - v1/zone/xx-yy.json (full info on the NxN rooms zone at coords xx,yy).
 */
class JsonMapStorage : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit JsonMapStorage(MapData &, const QString &, QObject *parent = nullptr);
    ~JsonMapStorage() override;

public:
    explicit JsonMapStorage() = delete;

private:
    virtual bool canLoad() const override { return false; }
    virtual bool canSave() const override { return true; }

    virtual void newData() override;
    virtual bool loadData() override;
    virtual bool saveData(bool baseMapOnly) override;
    virtual bool mergeData() override;

    // void saveMark(InfoMark * mark, QJsonObject &jRoom, const JsonRoomIdsCache &jRoomIds);
};
