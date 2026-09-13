#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <iterator>
#include <list>
#include <optional>

#include <QString>

// Command history used by InputWidget; has no QWidget dependency so it can be
// unit tested on its own.
class NODISCARD InputHistory final : private std::list<QString>
{
private:
    std::list<QString>::iterator m_iterator;

public:
    InputHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void backward() { std::advance(m_iterator, -1); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool atFront() const { return m_iterator == begin(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }
};

class NODISCARD TabHistory final : private std::list<QString>
{
    using base = std::list<QString>;

private:
    std::list<QString>::iterator m_iterator;

public:
    TabHistory() { m_iterator = begin(); }

public:
    void addInputLine(const QString &);

public:
    void forward() { std::advance(m_iterator, 1); }
    void reset() { m_iterator = begin(); }

public:
    NODISCARD const QString &value() const { return *m_iterator; }

public:
    NODISCARD bool empty() { return base::empty(); }
    NODISCARD bool atEnd() const { return m_iterator == end(); }

public:
    // Iterates from the current iterator; returns the next word starting with
    // fragment and advances past it; nullopt + reset() on wraparound (mirrors
    // InputWidget::tabComplete's end-of-dictionary behavior).
    NODISCARD std::optional<QString> nextMatch(const QString &fragment);
};
