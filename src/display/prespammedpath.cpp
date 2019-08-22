// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "prespammedpath.h"

#include <utility>

#include "../parser/CommandId.h"
#include "../parser/abstractparser.h"

// #define TEST
#ifdef TEST
static constexpr const bool USE_TEST = true;
#else
static constexpr const bool USE_TEST = false;
#endif

PrespammedPath::PrespammedPath(QObject * /*unused*/)
{
    if (USE_TEST) {
        m_queue.append(CommandIdType::DOWN);
        m_queue.append(CommandIdType::EAST);
        m_queue.append(CommandIdType::SOUTH);
        m_queue.append(CommandIdType::SOUTH);
        m_queue.append(CommandIdType::WEST);
        m_queue.append(CommandIdType::NORTH);
        m_queue.append(CommandIdType::WEST);
    }
}

PrespammedPath::~PrespammedPath() = default;

void PrespammedPath::setPath(CommandQueue queue, bool upd)
{
    m_queue = std::move(queue);
    if (upd) {
        emit update();
    }
}
