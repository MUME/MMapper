#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/utils.h"
#include "../map/CommandId.h"

#include <deque>

#include <QByteArray>

class NODISCARD CommandQueue final : private std::deque<CommandEnum>
{
private:
    using base = std::deque<CommandEnum>;

public:
    using base::base;

public:
    using base::begin;
    using base::empty;
    using base::end;
    using base::front;
    using base::size;

public:
    using base::clear;
    using base::pop_front;
    using base::push_back;
    using base::push_front;

public:
    NODISCARD CommandEnum take_front() { return utils::pop_front(*this); }

public:
    // DEPRECATED_MSG("use empty()")
    NODISCARD bool isEmpty() const { return empty(); }

    // DEPRECATED_MSG("use front()")
    NODISCARD CommandEnum head() const { return front(); }

public:
    // DEPRECATED_MSG("use push_back()")
    void enqueue(const CommandEnum cmd) { push_back(cmd); }

    // DEPRECATED_MSG("use take_front()")
    NODISCARD CommandEnum dequeue() { return take_front(); }

    // DEPRECATED_MSG("use push_back()")
    void append(const CommandEnum cmd) { push_back(cmd); }
};

namespace mmqt {
NODISCARD extern QByteArray toQByteArray(const CommandQueue &queue);
NODISCARD extern CommandQueue toCommandQueue(const QByteArray &dirs);
} // namespace mmqt
