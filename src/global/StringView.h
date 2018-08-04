#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef MMAPPER_STRINGVIEW_H
#define MMAPPER_STRINGVIEW_H

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

#endif // MMAPPER_STRINGVIEW_H
