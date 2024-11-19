// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "StringView.h"

#include "TextUtils.h"
#include "logging.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <QString>

StringView::StringView(const std::string_view sv) noexcept
    : m_sv{sv}
{}

StringView::StringView(const std::string &s) noexcept
    : m_sv{s}
{}

std::string StringView::toStdString() const
{
    return std::string(m_sv);
}

QString StringView::toQString() const
{
    return mmqt::toQStringLatin1(m_sv);
}

QByteArray StringView::toQByteArray() const
{
    return mmqt::toQByteArrayLatin1(m_sv);
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
        if (!isSpace(firstChar()))
            break;
    return *this;
}

StringView &StringView::trimRight() noexcept
{
    for (; !isEmpty(); eatLast())
        if (!isSpace(lastChar()))
            break;
    return *this;
}

void StringView::mustNotBeEmpty() const
{
    if (isEmpty())
        throw std::runtime_error("StringView is empty");
}

char StringView::firstChar() const
{
    mustNotBeEmpty();
    return *m_sv.cbegin();
}

char StringView::lastChar() const
{
    mustNotBeEmpty();
    return *m_sv.crbegin();
}

// is NOT allowed to be a space
char StringView::takeFirstLetter()
{
    mustNotBeEmpty();

    const auto c = firstChar();
    if (isSpace(c))
        throw std::runtime_error("space");

    m_sv.remove_prefix(1);
    return c;
}

StringView StringView::takeFirstWordNoPostTrim()
{
    trimLeft();
    mustNotBeEmpty();

    assert(!isSpace(lastChar()));

    size_t len = 0;
    const auto before = m_sv;
    for (; !isEmpty(); eatFirst(), ++len)
        if (isSpace(firstChar()))
            break;

    return StringView{before.substr(0, len)};
}

StringView StringView::takeFirstWord()
{
    auto result = takeFirstWordNoPostTrim();
    trimLeft();
    return result;
}

int StringView::countNonSpaceChars() const noexcept
{
    int result = 0;
    for (char c : m_sv)
        if (!isSpace(c))
            ++result;
    return result;
}

/* NOTE: This must can throw because it calls takeFirstWord() */
int StringView::countWords() const
{
    auto tmp = *this;
    tmp.trim();

    int result = 0;
    for (; !tmp.isEmpty(); std::ignore = tmp.takeFirstWord())
        ++result;
    return result;
}

std::vector<StringView> StringView::getWords() const
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

std::vector<std::string> StringView::getWordsAsStdStrings() const
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

std::vector<QString> StringView::getWordsAsQStrings() const
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

StringView StringView::substr(const size_t pos, const size_t len) const
{
    assert(pos <= m_sv.size() || pos == std::string_view::npos);
    assert(len <= m_sv.size() || len == std::string_view::npos);
    assert(pos + len <= m_sv.size() || pos == std::string_view::npos
           || len == std::string_view::npos);
    return StringView{m_sv.substr(pos, len)};
}

StringView StringView::left(const size_t len) const
{
    return substr(0, len);
}

StringView StringView::mid(const size_t pos) const
{
    return substr(pos);
}

StringView StringView::rmid(const size_t pos) const
{
    if (pos == std::string_view::npos)
        return *this;

    assert(pos <= m_sv.size());
    return substr(0, m_sv.size() - pos);
}

StringView StringView::right(const size_t len) const
{
    if (len == std::string_view::npos)
        return *this;

    assert(len <= m_sv.size());
    return substr(m_sv.size() - len);
}

bool StringView::startsWith(const std::string_view other) const
{
    if (m_sv.size() < other.size())
        return false;

    return left(other.size()) == other;
}

bool StringView::endsWith(const std::string_view other) const
{
    if (m_sv.size() < other.size())
        return false;

    return right(other.size()) == other;
}

void StringView::remove_suffix(const size_t n)
{
    assert(m_sv.size() >= n);
    m_sv.remove_suffix(n);
}

StringView &StringView::operator++()
{
    m_sv.remove_prefix(1);
    return *this;
}

char StringView::operator[](size_t pos) const
{
    return m_sv[pos];
}

namespace detail {

namespace intersection {

NODISCARD static constexpr bool intersects(const uintptr_t x, const uintptr_t lo, const uintptr_t hi)
{
    return lo <= hi && lo <= x && x <= hi;
}

NODISCARD static constexpr bool intersects(const uintptr_t abeg,
                                           const uintptr_t aend,
                                           const uintptr_t bbeg,
                                           const uintptr_t bend)
{
    if (abeg == aend) { // empty
        if (bbeg == bend) {
            return abeg == bbeg;
        }
        return ::detail::intersection::intersects(abeg, bbeg, bend);
    } else if (bbeg == bend) { // empty
        return ::detail::intersection::intersects(bbeg, abeg, aend);
    }

    return abeg <= aend && bbeg <= bend && abeg < bend && bbeg < aend;
}

NODISCARD static constexpr bool test_intersects(const uintptr_t abeg,
                                                const uintptr_t aend,
                                                const uintptr_t bbeg,
                                                const uintptr_t bend)
{
    return ::detail::intersection::intersects(abeg, aend, bbeg, bend)
           && ::detail::intersection::intersects(bbeg, bend, abeg, aend);
}

NODISCARD static constexpr bool test_does_not_intersect(const uintptr_t abeg,
                                                        const uintptr_t aend,
                                                        const uintptr_t bbeg,
                                                        const uintptr_t bend)
{
    return !::detail::intersection::intersects(abeg, aend, bbeg, bend)
           && !::detail::intersection::intersects(bbeg, bend, abeg, aend);
}

//
static_assert(::detail::intersection::test_intersects(0, 0, 0, 0)); // special case
static_assert(::detail::intersection::test_intersects(1, 1, 1, 1)); // special case
                                                                    //
static_assert(::detail::intersection::test_intersects(0, 0, 0, 1));
static_assert(::detail::intersection::test_intersects(0, 1, 0, 1));
static_assert(::detail::intersection::test_intersects(1, 1, 0, 1)); // special case
                                                                    //
static_assert(::detail::intersection::test_does_not_intersect(0, 0, 1, 2));
static_assert(::detail::intersection::test_does_not_intersect(0, 1, 1, 2));
static_assert(::detail::intersection::test_intersects(0, 2, 1, 2));
static_assert(::detail::intersection::test_intersects(1, 1, 1, 2));
static_assert(::detail::intersection::test_intersects(1, 2, 1, 2));
static_assert(::detail::intersection::test_intersects(2, 2, 1, 2)); // special case
static_assert(::detail::intersection::test_does_not_intersect(2, 3, 1, 2));
//
static_assert(::detail::intersection::test_intersects(0, 0, 0, 2));
static_assert(::detail::intersection::test_intersects(0, 1, 0, 2));
static_assert(::detail::intersection::test_intersects(0, 2, 0, 2));
static_assert(::detail::intersection::test_intersects(1, 1, 0, 2));
static_assert(::detail::intersection::test_intersects(1, 2, 0, 2));
static_assert(::detail::intersection::test_intersects(2, 2, 0, 2)); // special case
                                                                    //
static_assert(
    ::detail::intersection::test_does_not_intersect(0, 0, 1, 1)); // strictly non-intersecting
static_assert(::detail::intersection::test_does_not_intersect(
    0, 1, 1, 2)); // overlapping at non-included boundary
static_assert(::detail::intersection::test_does_not_intersect(1, 0, 0, 1)); // backwards range

NODISCARD static bool intersects(const char *const abeg,
                                 const char *const aend,
                                 const char *const bbeg,
                                 const char *const bend)
{
    return ::detail::intersection::intersects(reinterpret_cast<uintptr_t>(abeg),
                                              reinterpret_cast<uintptr_t>(aend),
                                              reinterpret_cast<uintptr_t>(bbeg),
                                              reinterpret_cast<uintptr_t>(bend));
}

NODISCARD static bool intersects(const char *const a,
                                 const size_t alen,
                                 const char *const b,
                                 const size_t blen)
{
    return ::detail::intersection::intersects(a, a + alen, b, b + blen);
}

} // namespace intersection

namespace substring {

// a is a substring of b
NODISCARD static constexpr bool isSubstringOf(const uintptr_t abeg,
                                              const uintptr_t aend,
                                              const uintptr_t bbeg,
                                              const uintptr_t bend)
{
    return abeg <= aend && bbeg <= bend && bbeg <= abeg && aend <= bend;
}

// a is a substring of b
NODISCARD static bool isSubstringOf(const char *const abeg,
                                    const char *const aend,
                                    const char *const bbeg,
                                    const char *const bend)
{
    return ::detail::substring::isSubstringOf(reinterpret_cast<uintptr_t>(abeg),
                                              reinterpret_cast<uintptr_t>(aend),
                                              reinterpret_cast<uintptr_t>(bbeg),
                                              reinterpret_cast<uintptr_t>(bend));
}

// a is a substring of b
NODISCARD static bool isSubstringOf(const char *const a,
                                    const size_t alen,
                                    const char *const b,
                                    const size_t blen)
{
    return ::detail::substring::isSubstringOf(a, a + alen, b, b + blen);
}
} // namespace substring

} // namespace detail

bool StringView::intersects(const StringView other) const
{
    return ::detail::intersection::intersects(m_sv.data(),
                                              m_sv.size(),
                                              other.m_sv.data(),
                                              other.m_sv.size());
}

bool StringView::isSubstringOf(const StringView other) const
{
    return ::detail::substring::isSubstringOf(m_sv.data(),
                                              m_sv.size(),
                                              other.m_sv.data(),
                                              other.m_sv.size());
}

StringView StringView::beforeSubstring(const StringView other) const
{
    assert(other.isSubstringOf(*this));
    const auto end = other.m_sv.data() - m_sv.data();
    assert(end >= 0 && static_cast<size_t>(end) <= m_sv.length());
    return left(static_cast<size_t>(end));
}

StringView StringView::startingWithSubstring(const StringView other) const
{
    assert(other.isSubstringOf(*this));
    const auto end = other.m_sv.data() - m_sv.data();
    assert(end >= 0 && static_cast<size_t>(end) <= m_sv.length());
    return mid(static_cast<size_t>(end));
}

StringView StringView::upToAndIncludingSubstring(const StringView other) const
{
    assert(other.isSubstringOf(*this));
    const auto end = other.m_sv.data() + other.m_sv.size();
    const auto offset = end - m_sv.data();
    assert(offset >= 0 && static_cast<size_t>(offset) <= m_sv.length());
    return left(static_cast<size_t>(offset));
}

StringView StringView::afterSubstring(const StringView other) const
{
    assert(other.isSubstringOf(*this));
    const auto end = other.m_sv.data() + other.m_sv.size();
    const auto offset = end - m_sv.data();
    assert(offset >= 0 && static_cast<size_t>(offset) <= m_sv.length());
    return mid(static_cast<size_t>(offset));
}

namespace { // anonymous
namespace test_detail {
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
    for (const char c : tmp) {
        ++count;
        static_cast<void>(c); /* unused */
    }
    TEST_ASSERT(count == 0);
    bool threwException = false;
    try {
        std::ignore = tmp.takeFirstLetter();
    } catch (...) {
        threwException = true;
    }
    TEST_ASSERT(threwException);
}

static void testLazyDog(const bool verbose = false)
{
    std::ostringstream os;
    const std::string s = "The quick brown fox\njumps \t\tover\t\t the lazy dog.\n";
    const auto view = StringView{s}.trim();
    const auto words = view.countWords();
    if (verbose)
        os << "# words: " << words << "\n";
    TEST_ASSERT(words == 9);

    const auto nonSpaceChars = view.countNonSpaceChars();
    if (verbose)
        os << "# non-space chars: " << nonSpaceChars << "\n";
    TEST_ASSERT(nonSpaceChars == 36);

    if (verbose)
        os << "---\n";
    int seenWords = 0;
    for (const auto &w : view.getWordsAsStdStrings()) {
        if (seenWords++ > 0) {
            if (verbose)
                os << " ";
        }
        if (verbose)
            os << "[" << w << "]";
    }
    TEST_ASSERT(seenWords == words);
    if (verbose) {
        os << "\n---\n";
        MMLOG() << std::move(os).str();
    }
}

static void testIntersect()
{
    const std::string s = "test";
    const StringView sv{s};
    assert(sv.intersects(sv.left(0)));      // "test" vs "" (at position 0)
    assert(sv.intersects(sv.substr(2, 0))); // "test" vs "" (at position 2)
    assert(sv.intersects(sv.right(0)));     // "test" vs "" (at position 4)
    //
    assert(sv.left(3).intersects(sv.right(2)));  // "tes" vs "st"
    assert(sv.left(2).intersects(sv.right(3)));  // "te" vs "est"
    assert(!sv.left(2).intersects(sv.right(2))); // "te" vs "st" does not intersect
    //
    assert(sv.left(0).intersects(sv.mid(0)));  // same as sv.intersects(sv.left(0));
    assert(!sv.left(1).intersects(sv.mid(1))); // "t" vs "est" does not intersect
    assert(!sv.left(2).intersects(sv.mid(2))); // "te" vs "st" does not intersect
    assert(!sv.left(3).intersects(sv.mid(3))); // "tes" vs "t" does not intersect
    assert(sv.left(4).intersects(sv.mid(4)));  // same as sv.intersects(sv.right(0));
}

static void testSubstring()
{
    const std::string s = "test";
    const StringView sv{s};
    assert(sv.isSubstringOf(sv));

    assert(!sv.isSubstringOf(sv.left(0)));
    assert(!sv.isSubstringOf(sv.substr(2, 0)));
    assert(!sv.isSubstringOf(sv.right(0)));

    assert(sv.left(0).isSubstringOf(sv.left(0)));
    assert(sv.substr(2, 0).isSubstringOf(sv));
    assert(sv.right(0).isSubstringOf(sv));
}

} // namespace test_detail
} // namespace

void test::testStringView()
{
    using namespace test_detail;
    testEmpty();
    testLazyDog();
    testIntersect();
    testSubstring();
    MMLOG() << "Test \"" << __FUNCTION__ << "\" passed.";
}
