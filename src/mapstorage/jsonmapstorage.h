#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "abstractmapstorage.h"

#include <QString>
#include <QtCore>

class MapData;
class QObject;

/*! \brief JSON export for web clients.
 *
 * This saves to a directory the following files:
 * - v1/arda.json (global metadata like map size).
 * - v1/roomindex/ss.json (room sums -> zone coords).
 * - v1/zone/xx-yy.json (full info on the NxN rooms zone at coords xx,yy).
 */
class JsonMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit JsonMapStorage(MapData &, const QString &, QObject *parent);
    ~JsonMapStorage() final;

public:
    JsonMapStorage() = delete;

private:
    NODISCARD bool canLoad() const override { return false; }
    NODISCARD bool canSave() const override { return true; }

    void newData() override;
    NODISCARD bool loadData() override;
    NODISCARD bool saveData(bool baseMapOnly) override;
    NODISCARD bool mergeData() override;
    void log(const QString &msg) { emit sig_log("JsonMapStorage", msg); }
    // void saveMark(InfoMark * mark, QJsonObject &jRoom, const JsonRoomIdsCache &jRoomIds);
};
