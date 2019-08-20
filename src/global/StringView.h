#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <string>
#include <string_view>
#include <vector>
#include <QByteArray>
#include <QChar>
#include <QString>

/*
 * NOTE: As a view, it requires the string object to remain valid and unchanged for the entire lifetime.
 *
 * NOTE: This is does not fill the same role as QStringView or std::string_view.
 */
struct StringView final
{
public:
    using const_iterator = std::string_view::const_iterator;

private:
    std::string_view m_sv;

public:
    StringView() noexcept = default;
    explicit StringView(const std::string_view &sv) noexcept;
    explicit StringView(const std::string &s) noexcept;
    explicit StringView(std::string &&) = delete;
    explicit StringView(const QString &) = delete;

public:
    inline const_iterator begin() const noexcept { return m_sv.begin(); }
    inline const_iterator end() const noexcept { return m_sv.end(); }
    inline auto size() const noexcept { return static_cast<int>(m_sv.size()); }
    inline bool isEmpty() const noexcept { return empty(); }
    inline bool empty() const noexcept { return m_sv.empty(); }

public:
    std::string toStdString() const noexcept(false);
    QString toQString() const noexcept(false);
    QByteArray toQByteArray() const noexcept(false);

public:
    StringView &trimLeft() noexcept;
    StringView &trimRight() noexcept;
    inline StringView &trim() noexcept { return trimLeft().trimRight(); }

private:
    void mustNotBeEmpty() const noexcept(false);
    void eatFirst();
    void eatLast();

public:
    char firstChar() const noexcept(false);
    char lastChar() const noexcept(false);

public:
    char takeFirstLetter() noexcept(false);

public:
    StringView takeFirstWord() noexcept(false);

public:
    int countNonSpaceChars() const noexcept;
    int countWords() const noexcept(false);
    std::vector<StringView> getWords() const noexcept(false);
    std::vector<std::string> getWordsAsStdStrings() const noexcept(false);
    std::vector<QString> getWordsAsQStrings() const noexcept(false);

public:
    bool operator==(const char *s) const noexcept;
};

namespace test {
void testStringView();
} // namespace test
