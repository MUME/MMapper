#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include "../global/JsonObj.h"
#include "RoomMob.h"
#include "RoomMobs.h"

#include <QObject>

class RoomMobUpdate;
class GmcpMessage;

class NODISCARD_QOBJECT RoomManager final : public QObject
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

private:
    void parseGmcpAdd(const GmcpMessage &msg);
    void parseGmcpRemove(const GmcpMessage &msg);
    void parseGmcpSet(const GmcpMessage &msg);
    void parseGmcpUpdate(const GmcpMessage &msg);

    void showGmcp(const GmcpMessage &msg) const;

    void addMob(const JsonObj &obj);
    void updateMob(const JsonObj &obj);
    void updateWidget();

    NODISCARD bool toMob(const JsonObj &obj, RoomMobUpdate &mob) const;
    static void toMobField(const JsonValue &value, RoomMobUpdate &data, MobFieldEnum i);

signals:
    void sig_updateWidget(); // update RoomWidget

public slots:
    void slot_reset();
    void slot_parseGmcpInput(const GmcpMessage &msg);
};
