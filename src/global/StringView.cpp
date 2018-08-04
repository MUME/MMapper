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

#include "StringView.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <QString>

StringView::StringView(StringView::const_iterator const beg,
                       StringView::const_iterator const end) noexcept
    : begin_{beg}
    , end_{end}
{}

StringView::StringView(const QString &s) noexcept(false)
    : StringView{s.constBegin(), s.constEnd()}
{}

QString StringView::toQString() const noexcept(false)
{
    return QString{(QChar *) begin_, size()};
}

QByteArray StringView::toQByteArray() const noexcept(false)
{
    return toQString().toLatin1();
}

StringView &StringView::trimLeft() noexcept
{
    for (; begin_ < end_; ++begin_)
        if (!firstChar().isSpace())
            break;
    return *this;
}

StringView &StringView::trimRight() noexcept
{
    for (; begin_ < end_; --end_)
        if (!lastChar().isSpace())
            break;
    return *this;
}

void StringView::mustNotBeEmpty() const noexcept(false)
{
    if (isEmpty() || !(begin_ < end_))
        throw std::runtime_error("StringView is empty");
}

QChar StringView::firstChar() const noexcept(false)
{
    mustNotBeEmpty();
    return *begin_;
}

QChar StringView::lastChar() const noexcept(false)
{
    mustNotBeEmpty();
    return *(end_ - 1);
}

QChar StringView::takeFirstLetter() noexcept(false)
{
    mustNotBeEmpty();

    const auto c = firstChar();
    if (c.isSpace())
        throw std::runtime_error("space");

    ++begin_;
    return c;
}

StringView StringView::takeFirstWord() noexcept(false)
{
    trimLeft();
    mustNotBeEmpty();

    assert(!lastChar().isSpace());

    const auto it = begin_;
    for (; begin_ != end_; ++begin_)
        if (firstChar().isSpace())
            break;

    const auto result = StringView{it, begin_};
    trimLeft();
    return result;
}

int StringView::countNonSpaceChars() const noexcept
{
    int result = 0;
    for (auto it = begin_; it != end_; ++it)
        if (!it->isSpace())
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
    std::vector<StringView> result{};
    result.reserve(numWords);

    auto tmp = *this;
    tmp.trim();

    while (!tmp.isEmpty())
        result.emplace_back(tmp.takeFirstWord());

    assert(static_cast<int>(result.size()) == numWords);
    return result;
}

std::vector<QString> StringView::getWordsAsQStrings() const noexcept(false)
{
    const auto numWords = countWords();
    std::vector<QString> result{};
    result.reserve(numWords);

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
    StringView tmp{};
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
    const QString s = "The quick brown fox\njumps \t\tover\t\t the lazy dog.\n";
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
    for (const auto &w : view.getWordsAsQStrings()) {
        if (seenWords++ > 0) {
            if (verbose)
                std::cout << " ";
        }
        if (verbose)
            std::cout << "[" << w.toStdString() << "]";
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
    std::cout << "Test \"" << __PRETTY_FUNCTION__ << "\" passed.\n" << std::flush;
    std::cerr << std::flush;
}

} // namespace test
