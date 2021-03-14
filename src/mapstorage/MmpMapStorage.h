#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QtCore>

#include "../global/macros.h"
#include "abstractmapstorage.h"

class MapData;
class QObject;
class QXmlStreamWriter;

/*! \brief MMP export for other clients
 *
 * This saves to a XML file following the MMP Specification defined at:
 * https://wiki.mudlet.org/w/Standards:MMP
 */
class MmpMapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MmpMapStorage(MapData &, const QString &, QFile *, QObject *parent);
    ~MmpMapStorage() final;

public:
    MmpMapStorage() = delete;

private:
    NODISCARD virtual bool canLoad() const override { return false; }
    NODISCARD virtual bool canSave() const override { return true; }

    virtual void newData() override;
    NODISCARD virtual bool loadData() override;
    NODISCARD virtual bool saveData(bool baseMapOnly) override;
    NODISCARD virtual bool mergeData() override;

private:
    static void saveRoom(const Room &room, QXmlStreamWriter &stream);
    void log(const QString &msg) { emit sig_log("MmpMapStorage", msg); }
};
