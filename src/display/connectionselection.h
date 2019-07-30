#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <array>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h" /* Coordinate2f */
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class MapFrontend;
class Room;
class RoomAdmin;
struct RoomId;

struct MouseSel final
{
    Coordinate2f pos;
    int layer = 0;

    MouseSel() = default;
    explicit MouseSel(const Coordinate2f &pos, const int layer)
        : pos{pos}
        , layer{layer}
    {}

    Coordinate getCoordinate() const { return Coordinate{pos.round(), layer}; }
    Coordinate getScaledCoordinate(const float xy_scale) const
    {
        return Coordinate{(pos * xy_scale).round(), layer};
    }
};

class ConnectionSelection final : public QObject, public RoomRecipient
{
    Q_OBJECT

public:
    struct ConnectionDescriptor final
    {
        const Room *room = nullptr;
        ExitDirEnum direction = ExitDirEnum::NONE;
    };

    explicit ConnectionSelection(MapFrontend *mf, const MouseSel &sel);
    ConnectionSelection();
    ~ConnectionSelection() override;

    void setFirst(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, RoomId RoomId, ExitDirEnum dir);
    void setSecond(MapFrontend *mf, const MouseSel &sel);
    void removeSecond();

    ConnectionDescriptor getFirst();
    ConnectionDescriptor getSecond();

    bool isValid();
    bool isFirstValid() { return m_connectionDescriptor[0].room != nullptr; }
    bool isSecondValid() { return m_connectionDescriptor[1].room != nullptr; }

    void receiveRoom(RoomAdmin *admin, const Room *aRoom) override;

public slots:

signals:

protected:
private:
    static ExitDirEnum ComputeDirection(const Coordinate2f &mouse_f);

    // REVISIT: give these enum names?
    MMapper::Array<ConnectionDescriptor, 2> m_connectionDescriptor;

    bool m_first = true;
    RoomAdmin *m_admin = nullptr;
};
