// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CommandQueue.h"

#include "../map/ExitDirection.h"

NODISCARD static CommandEnum toCommandEnum(const ExitDirEnum dir)
{
#define CASE(X) \
    case ExitDirEnum::X: \
        return CommandEnum::X
    switch (dir) {
        CASE(NORTH);
        CASE(SOUTH);
        CASE(EAST);
        CASE(WEST);
        CASE(UP);
        CASE(DOWN);
        CASE(UNKNOWN);
        CASE(NONE);
    default:
        break;
    }

    return CommandEnum::UNKNOWN;
}

namespace mmqt {
QByteArray toQByteArray(const CommandQueue &queue)
{
    QByteArray dirs;
    for (const CommandEnum cmd : queue) {
        // REVISIT: Serialize/deserialize directions more intelligently
        const ExitDirEnum dir = getDirection(cmd);
        dirs.append(Mmapper2Exit::charForDir(dir));
    }
    return dirs;
}

CommandQueue toCommandQueue(const QByteArray &dirs)
{
    CommandQueue queue;
    for (const char c : dirs) {
        const ExitDirEnum dir = Mmapper2Exit::dirForChar(c);
        queue.push_back(toCommandEnum(dir));
    }
    return queue;
}

} // namespace mmqt
