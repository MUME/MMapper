// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "StringView.h"

#include <cassert>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <QString>

#include "TextUtils.h"

static bool is_space(char c)
{
    return std::isspace(static_cast<uint8_t>(c) & 0xff) || c == C_NBSP;
}

StringView::StringView(const std::string_view &sv) noexcept
    : m_sv{sv}
{}

StringView::StringView(const std::string &s) noexcept
    : m_sv{s}
{}

std::string StringView::toStdString() const noexcept(false)
{
    return std::string(m_sv);
}

QString StringView::toQString() const noexcept(false)
{
    return QString::fromStdString(toStdString());
}

QByteArray StringView::toQByteArray() const noexcept(false)
{
    return QByteArray(m_sv.data(), static_cast<int>(m_sv.size()));
}

void StringView::eatFirst()
{
    assert(!isEmpty());
    m_sv.remove_prefix(1);
}

void StringView::eatLast()
{
    assert(!isEmpty());
    m_sv.remove_suffix(1);
}

StringView &StringView::trimLeft() noexcept
{
    for (; !isEmpty(); eatFirst())
        if (!is_space(firstChar()))
            break;
    return *this;
}

StringView &StringView::trimRight() noexcept
{
    for (; !isEmpty(); eatLast())
        if (!is_space(lastChar()))
            break;
    return *this;
}

void StringView::mustNotBeEmpty() const noexcept(false)
{
    if (isEmpty())
        throw std::runtime_error("StringView is empty");
}

char StringView::firstChar() const noexcept(false)
{
    mustNotBeEmpty();
    return *m_sv.cbegin();
}

char StringView::lastChar() const noexcept(false)
{
    mustNotBeEmpty();
    return *m_sv.crbegin();
}

// is NOT allowed to be a space
char StringView::takeFirstLetter() noexcept(false)
{
    mustNotBeEmpty();

    const auto c = firstChar();
    if (is_space(c))
        throw std::runtime_error("space");

    m_sv.remove_prefix(1);
    return c;
}

StringView StringView::takeFirstWord() noexcept(false)
{
    trimLeft();
    mustNotBeEmpty();

    assert(!is_space(lastChar()));

    size_t len = 0;
    const auto before = m_sv;
    for (; !isEmpty(); eatFirst(), ++len)
        if (is_space(firstChar()))
            break;

    trimLeft();
    return StringView{before.substr(0, len)};
}

int StringView::countNonSpaceChars() const noexcept
{
    int result = 0;
    for (char c : m_sv)
        if (!is_space(c))
            ++result;
    return result;
}

/* NOTE: This must be flagged noexcept(false) because it calls takeFirstWord() */
int StringView::countWords() const noexcept(false)
{
    auto tmp = *this;
    tmp.trim();

    int result = 0;
    for (; !tmp.isEmpty(); tmp.takeFirstWord())
        ++result;
    return result;
}

std::vector<StringView> StringView::getWords() const noexcept(false)
{
    const auto numWords = countWords();
    assert(numWords >= 0);
    std::vector<StringView> result{};
    result.reserve(static_cast<size_t>(numWords));

    auto tmp = *this;
    tmp.trim();

    while (!tmp.isEmpty())
        result.emplace_back(tmp.takeFirstWord());

    assert(static_cast<int>(result.size()) == numWords);
    return result;
}

std::vector<std::string> StringView::getWordsAsStdStrings() const noexcept(false)
{
    const auto numWords = countWords();
    assert(numWords >= 0);
    std::vector<std::string> result{};
    result.reserve(static_cast<size_t>(numWords));

    auto tmp = *this;
    tmp.trim();

    while (!tmp.isEmpty())
        result.emplace_back(tmp.takeFirstWord().m_sv);

    assert(static_cast<int>(result.size()) == numWords);
    return result;
}

std::vector<QString> StringView::getWordsAsQStrings() const noexcept(false)
{
    const auto numWords = countWords();
    assert(numWords >= 0);
    std::vector<QString> result{};
    result.reserve(static_cast<size_t>(numWords));

    auto tmp = *this;
    tmp.trim();

    while (!tmp.isEmpty())
        result.emplace_back(tmp.takeFirstWord().toQString());

    assert(static_cast<int>(result.size()) == numWords);
    return result;
}

bool StringView::operator==(const char *s) const noexcept
{
    if (s == nullptr)
        return isEmpty();

    if (isEmpty())
        return false;

    auto tmp = *this;
    while (!tmp.isEmpty() && *s != '\0') // both are non-empty
        if (tmp.takeFirstLetter() != *s++)
            return false;

    return tmp.isEmpty() && (*s == '\0'); // both are empty
}

namespace test {
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define TEST_ASSERT(x) \
    do { \
        if (!(x)) \
            throw std::runtime_error("test failure in at " __FILE__ \
                                     ":" STRINGIZE(__LINE__) ": " #x); \
    } while (false)

static void testEmpty()
{
    StringView tmp;
    TEST_ASSERT(tmp.empty());
    int count = 0;
    for (auto c : tmp) {
        ++count;
        static_cast<void>(c); /* unused */
    }
    TEST_ASSERT(count == 0);
    bool threwException = false;
    try {
        tmp.takeFirstLetter();
    } catch (...) {
        threwException = true;
    }
    TEST_ASSERT(threwException);
}

static void testLazyDog(const bool verbose = false)
{
    const std::string s = "The quick brown fox\njumps \t\tover\t\t the lazy dog.\n";
    const auto view = StringView{s}.trim();
    const auto words = view.countWords();
    if (verbose)
        std::cout << "# words: " << words << "\n";
    TEST_ASSERT(words == 9);

    const auto nonSpaceChars = view.countNonSpaceChars();
    if (verbose)
        std::cout << "# non-space chars: " << nonSpaceChars << "\n";
    TEST_ASSERT(nonSpaceChars == 36);

    if (verbose)
        std::cout << "---\n";
    int seenWords = 0;
    for (const auto &w : view.getWordsAsStdStrings()) {
        if (seenWords++ > 0) {
            if (verbose)
                std::cout << " ";
        }
        if (verbose)
            std::cout << "[" << w << "]";
    }
    TEST_ASSERT(seenWords == words);
    if (verbose) {
        std::cout << "\n---\n";
        std::cout << std::flush;
        std::cerr << std::flush;
    }
}

void testStringView()
{
    testEmpty();
    testLazyDog();
    std::cout << "Test \"" << __FUNCTION__ << "\" passed.\n" << std::flush;
    std::cerr << std::flush;
}

} // namespace test
