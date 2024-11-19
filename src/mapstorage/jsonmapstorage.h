#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "abstractmapstorage.h"

#include <optional>

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
class NODISCARD_QOBJECT JsonMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit JsonMapStorage(const AbstractMapStorage::Data &data, QObject *parent);
    ~JsonMapStorage() final;

public:
    JsonMapStorage() = delete;

private:
    NODISCARD bool virt_canLoad() const final { return false; }
    NODISCARD std::optional<RawMapLoadData> virt_loadData() final { return std::nullopt; }

private:
    NODISCARD bool virt_canSave() const final { return true; }
    NODISCARD bool virt_saveData(const RawMapData &map) final;

private:
    void log(const QString &msg) { emit sig_log("JsonMapStorage", msg); }
};
