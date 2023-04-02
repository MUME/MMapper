// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Massimiliano Ghilardi <massimiliano.ghilardi@gmail.com> (Cosmos)

#include <QDataStream>

#include "roomserverid.h"

QDataStream &operator<<(QDataStream &os, const RoomServerId &id)
{
    const bool is_set = id.isSet();
    os << is_set;
    if (is_set) {
        os << static_cast<quint64>(id.asUint64());
    }
    return os;
}

QDataStream &operator>>(QDataStream &os, RoomServerId &id)
{
    bool is_set = false;
    os >> is_set;
    if (is_set) {
        quint64 value = 0;
        os >> value;
        id = RoomServerId{static_cast<uint64_t>(value)};
    } else {
        id = RoomServerId{};
    }
    return os;
}
