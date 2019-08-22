#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <vector>
#include <QByteArray>
#include <QChar>
#include <QString>

/*
 * NOTE: As a view, it requires the string object to remain valid and unchanged for the entire lifetime.
 *
 * NOTE: This is does not fill the same role as QStringView.
 * TODO: Convert this to 8bit and use std::string instead of QString,
 * and allow views of string constants and character arrays.
 */
struct StringView
{
public:
    using const_iterator = QString::const_iterator;

private:
    const_iterator begin_ = nullptr;
    const_iterator end_ = nullptr;

public:
    explicit StringView() noexcept = default;
    explicit StringView(const const_iterator beg, const const_iterator end) noexcept;
    explicit StringView(const QString &s) noexcept(false);

public:
    inline const_iterator begin() const noexcept { return begin_; }
    inline const_iterator end() const noexcept { return end_; }
    inline auto size() const noexcept { return static_cast<int>(end_ - begin_); }
    inline bool isEmpty() const noexcept { return begin_ == end_; }
    inline bool empty() const noexcept { return isEmpty(); }

public:
    QString toQString() const noexcept(false);
    QByteArray toQByteArray() const noexcept(false);

public:
    StringView &trimLeft() noexcept;
    StringView &trimRight() noexcept;
    inline StringView &trim() noexcept { return trimLeft().trimRight(); }

private:
    void mustNotBeEmpty() const noexcept(false);

public:
    QChar firstChar() const noexcept(false);
    QChar lastChar() const noexcept(false);

public:
    QChar takeFirstLetter() noexcept(false);

public:
    StringView takeFirstWord() noexcept(false);

public:
    int countNonSpaceChars() const noexcept;
    int countWords() const noexcept(false);
    std::vector<StringView> getWords() const noexcept(false);
    std::vector<QString> getWordsAsQStrings() const noexcept(false);

public:
    bool operator==(const char *s) const noexcept;
};

namespace test {
void testStringView();
} // namespace test
