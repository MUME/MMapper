#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/RawRoom.h"
#include "abstractmapstorage.h"

#include <optional>

#include <QString>
#include <QtCore>

class MapData;
class QObject;
class QXmlStreamWriter;

/*! \brief MMP export for other clients
 *
 * This saves to a XML file following the MMP Specification defined at:
 * https://wiki.mudlet.org/w/Standards:MMP
 */
class NODISCARD_QOBJECT MmpMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MmpMapStorage(const AbstractMapStorage::Data &, QObject *parent);
    ~MmpMapStorage() final;

public:
    MmpMapStorage() = delete;

private:
    NODISCARD bool virt_canLoad() const final { return false; }
    NODISCARD std::optional<RawMapLoadData> virt_loadData() final { return std::nullopt; }

private:
    NODISCARD bool virt_canSave() const final { return true; }
    NODISCARD bool virt_saveData(const RawMapData &map) final;

private:
    static void saveRoom(const ExternalRawRoom &room, QXmlStreamWriter &stream);
    void log(const QString &msg) { emit sig_log("MmpMapStorage", msg); }
};
