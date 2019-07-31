#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QQueue>

#include "CommandId.h"

class CommandQueue : private QQueue<CommandEnum>
{
private:
    using base = QQueue<CommandEnum>;

public:
    using QQueue<CommandEnum>::QQueue;

public:
    QByteArray toByteArray() const;
    CommandQueue &operator=(const QByteArray &dirs);

public:
    using base::begin;
    using base::end;
    using base::head;
    using base::isEmpty;

public:
    using base::append;
    using base::clear;
    using base::dequeue;
    using base::enqueue;
    using base::prepend;
};
