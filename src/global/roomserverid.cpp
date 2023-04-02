// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <QDataStream>

#include "roomserverid.h"

QDataStream &operator<<(QDataStream &os, RoomServerId id)
{
    return os << static_cast<quint32>(id.asUint32());
}

QDataStream &operator>>(QDataStream &os, RoomServerId &id)
{
    quint32 value = 0;
    os >> value;
    id = RoomServerId{static_cast<uint32_t>(value)};
    return os;
}
