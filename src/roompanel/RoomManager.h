#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "RoomMob.h"
#include "RoomMobs.h"

#include <memory>

#include <QJsonObject>
#include <QObject>

class RoomMobUpdate;
class GmcpMessage;

class RoomManager final : public QObject
{
    Q_OBJECT

private:
    static const QHash<QString, MobFieldEnum> mobFields;

private:
    RoomMobs m_room;
    bool m_debug = false;

public:
    explicit RoomManager(QObject *parent);
    ~RoomManager() final;

public:
    NODISCARD const RoomMobs &getRoom() const { return m_room; }

signals:
    void sig_updateWidget(); // update RoomWidget

public slots:
    void slot_reset();
    void slot_parseGmcpInput(const GmcpMessage &msg);

private:
    void parseGmcpAdd(const GmcpMessage &msg);
    void parseGmcpRemove(const GmcpMessage &msg);
    void parseGmcpSet(const GmcpMessage &msg);
    void parseGmcpUpdate(const GmcpMessage &msg);

    void showGmcp(const GmcpMessage &msg) const;

    void addMob(const QJsonObject &obj);
    void updateMob(const QJsonObject &obj);
    void updateWidget();

    NODISCARD bool toMob(const QJsonObject &obj, RoomMobUpdate &mob) const;
    NODISCARD static bool toMobId(const QJsonValue &value, RoomMobUpdate &data);
    static void toMobField(const QJsonValue &value, RoomMobUpdate &data, MobFieldEnum i);
};
