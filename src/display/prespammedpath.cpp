// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "prespammedpath.h"

#include "../parser/CommandId.h"
#include "../parser/abstractparser.h"

#include <utility>

// #define TEST
#ifdef TEST
static constexpr const bool USE_TEST = true;
#else
static constexpr const bool USE_TEST = false;
#endif

PrespammedPath::PrespammedPath(QObject *parent)
    : QObject(parent)
{
    if (USE_TEST) {
        m_queue.append(CommandEnum::DOWN);
        m_queue.append(CommandEnum::EAST);
        m_queue.append(CommandEnum::SOUTH);
        m_queue.append(CommandEnum::SOUTH);
        m_queue.append(CommandEnum::WEST);
        m_queue.append(CommandEnum::NORTH);
        m_queue.append(CommandEnum::WEST);
    }
}

PrespammedPath::~PrespammedPath() = default;

void PrespammedPath::slot_setPath(CommandQueue queue)
{
    m_queue = std::move(queue);
    emit sig_update();
}
