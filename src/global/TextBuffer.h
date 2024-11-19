#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "TextUtils.h"
#include "macros.h"

#include <QString>

namespace mmqt {
class NODISCARD TextBuffer final
{
private:
    QString m_text;

public:
    void reserve(int len) { m_text.reserve(len); }
    NODISCARD const QString &getQString() const { return m_text; }
    NODISCARD auto length() const { return m_text.length(); }

public:
    void append(char c) { m_text.append(c); }
    void append(QChar c) { m_text.append(c); }
    void append(const QString &line) { m_text.append(line); }
    void append(const QStringView line) { m_text.append(line); }
    void appendUf8(std::string_view sv) { m_text.append(mmqt::toQStringUtf8(sv)); }

public:
    void appendJustified(QStringView line, int maxLen);
    void appendExpandedTabs(QStringView line, int start_at = 0);

public:
    NODISCARD bool isEmpty() const;
    NODISCARD bool hasTrailingNewline() const;
};
} // namespace mmqt
