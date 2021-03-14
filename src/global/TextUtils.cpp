// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "TextUtils.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <QRegularExpression>
#include <QString>

#include "utils.h"

// allows ">" or "|" as the quote character
static const QRegularExpression quotePrefixRegex(R"(^[[:space:]]*([>|][[:space:]]*)*)");

// literal "*" or "-" bullets
// numbered list "1." or "1)" .. "99." or "99)" (have to draw the line somewhere).
// lettered list "a." or "a)"
static const QRegularExpression bulletPrefixRegex(
    R"(^([*]|[-]|[[:alnum:]][.)]|[[1-9][[:digit:]][.)]))");
static const QRegularExpression leadingWhitespaceRegex(R"(^[[:space:]]+)");
static const QRegularExpression trailingWhitespaceRegex(R"([[:space:]]+$)");
static const QRegularExpression leadingNonSpaceRegex(R"(^[^[:space:]]+)");
/// only matches actual space characters
static const QRegularExpression twoOrMoreSpaceCharsRegex(R"(  +)");

const QRegularExpression weakAnsiRegex(R"(\x1b\[?[[:digit:];]*[[:alpha:]]?)");

static constexpr const char C_ANSI_ESCAPE = C_ESC;

bool containsAnsi(const QStringRef &str)
{
    return str.contains(C_ANSI_ESCAPE);
}

bool containsAnsi(const QString &str)
{
    return str.contains(C_ANSI_ESCAPE);
}

bool isAnsiColor(QStringRef ansi)
{
    if (ansi.length() < 3 || ansi.at(0) != C_ANSI_ESCAPE || ansi.at(1) != C_OPEN_BRACKET
        || ansi.at(ansi.length() - 1) != 'm') {
        return false;
    }

    ansi = ansi.mid(2, ansi.length() - 3);
    for (auto &c : ansi)
        // REVISIT: This may be incorrect if it allows Unicode digits
        // that are not part of the LATIN1 subset.
        if (c != C_SEMICOLON && !c.isDigit())
            return false;

    return true;
}

bool isAnsiColor(const QString &ansi)
{
    return isAnsiColor(ansi.midRef(0));
}

static int parsePositiveInt(const QStringRef &number)
{
    static constexpr int MAX = std::numeric_limits<int>::max();
    static_assert(MAX == 2147483647);
    static constexpr int LIMIT = MAX / 10;
    static_assert(LIMIT == 214748364);

    int n = 0;
    for (const QChar &qc : number) {
        // bounds check for sanity
        if (qc.unicode() > 0xff)
            return -1;

        const char c = qc.toLatin1();
        if (c < '0' || c > '9')
            return -1;

        const int digit = c - '0';

        // test if it's safe to multiply without integer overflow
        if (n > LIMIT)
            return -1;

        n *= 10;

        // test if it's safe to add without integer overflow.
        // e.g. This fails if n == 2147483640, and digit == 8,
        // since 2147483647 - 8 == 2147483639.
        if (n > MAX - digit)
            return -1;

        n += digit;
    }

    return n;
}

void AnsiColorParser::for_each(QStringRef ansi) const
{
    if (!isAnsiColor(ansi)) {
        // It's okay for this to be something that's not an ansi color.
        // For example, if someone types "\033[42am" in the editor
        // and then normalizes it, you'll get "\033[42a" as the escape code.
        return;
    }

    // NOTE: ESC[m is a special case alias for ESC[0m according to wikipedia,
    // but xterm also supports empty values like ESC[;42;m,
    // which gets interpreted as if you had used ESC[0;42;0m,
    // so mmapper will do the same.

    // ESC[...m -> ...
    ansi = ansi.mid(2, ansi.length() - 3);

    const int len = ansi.length();
    int pos = 0;

    const auto try_report = [this, &ansi, &pos](const int idx) -> void {
        if (idx <= pos)
            report(0);
        else
            report(parsePositiveInt(ansi.mid(pos, idx - pos)));
    };

    while (pos < len) {
        const int idx = ansi.indexOf(C_SEMICOLON, pos);
        if (idx < 0)
            break;

        try_report(idx);
        pos = idx + 1;
    }

    try_report(len);
}

int countLines(const QStringRef &input)
{
    int count = 0;
    foreachLine(input, [&count](const QStringRef &, bool) { ++count; });
    return count;
}

int countLines(const QString &input)
{
    return countLines(input.midRef(0));
}

int measureExpandedTabsOneLine(const QStringRef &line, const int startingColumn)
{
    int col = startingColumn;
    for (const auto &c : line) {
        assert(c != '\n');
        if (c == '\t') {
            col += 8 - (col % 8);
        } else {
            col += 1;
        }
    }
    return col;
}

int measureExpandedTabsOneLine(const QString &line, const int startingColumn)
{
    return measureExpandedTabsOneLine(line.midRef(0), startingColumn);
}

int measureExpandedTabsMultiline(const QStringRef &stringRef)
{
    int len = 0;
    foreachLine(stringRef, [&len](const QStringRef &line, bool hasNewline) {
        len += measureExpandedTabsOneLine(line, 0);
        if (hasNewline)
            len += 1;
    });
    return len;
}

int measureExpandedTabsMultiline(const QString &string)
{
    return measureExpandedTabsMultiline(string.midRef(0));
}

int findTrailingWhitespace(const QStringRef &line)
{
    auto m = trailingWhitespaceRegex.match(line);
    if (!m.hasMatch())
        return -1;
    return m.capturedStart();
}

int findTrailingWhitespace(const QString &line)
{
    return findTrailingWhitespace(line.midRef(0));
}

static constexpr const int ANSI_RESET = 0;
#define X(UPPER, lower, n) \
    static constexpr const int ANSI_##UPPER{n}; \
    static constexpr const int ANSI_##UPPER##_OFF{20 + (n)};
XFOREACH_ANSI_BIT(X)
#undef X
static constexpr const int ANSI_FG_COLOR = 30;
static constexpr const int ANSI_FG_DEFAULT = 39;
static constexpr const int ANSI_BG_COLOR = 40;
static constexpr const int ANSI_BG_DEFAULT = 49;
static constexpr const int ANSI_FG_COLOR_HI = 90;
static constexpr const int ANSI_BG_COLOR_HI = 100;

#define XFOREACH_ANSI_COLOR(X) \
    X(0, black) \
    X(1, red) \
    X(2, green) \
    X(3, yellow) \
    X(4, blue) \
    X(5, magenta) \
    X(6, cyan) \
    X(7, white)

AnsiString::AnsiString()
{
    // should fit in small string optimization
    buffer_.reserve(8);
    assert(isEmpty());
}

void AnsiString::add_code(const int code)
{
    if (buffer_.empty()) {
        buffer_ += C_ANSI_ESCAPE;
        buffer_ += C_OPEN_BRACKET;
    } else {
        assert(buffer_.back() == 'm');
        buffer_.back() = C_SEMICOLON;
    }

    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%dm", code);
    buffer_ += tmp;
}

/// e.g. ESC[1m -> ESC[0;1m
AnsiString AnsiString::copy_as_reset() const
{
    if (size() >= 3 && buffer_[2] == '0') {
        assert(buffer_[0] == C_ANSI_ESCAPE);
        assert(buffer_[1] == C_OPEN_BRACKET);
        /* already starts with ESC[0 */
        return *this;
    }

    AnsiString result;
    result.add_code(ANSI_RESET);
    if (size() >= 2) {
        assert(buffer_[0] == C_ANSI_ESCAPE);
        assert(buffer_[1] == C_OPEN_BRACKET);
        assert(result.size() == 4);
        assert(result.buffer_[0] == C_ANSI_ESCAPE);
        assert(result.buffer_[1] == C_OPEN_BRACKET);
        assert(result.buffer_[2] == '0');
        assert(result.buffer_[3] == 'm');
        result.buffer_.pop_back(); // 'm'
        result.buffer_ += C_SEMICOLON;
        result.buffer_ += buffer_.c_str() + 2; /* skip ESC[ */
        assert(result.size() == size() + 2);
    }
    return result;
}

AnsiString AnsiString::get_reset_string()
{
    AnsiString reset;
    reset.add_code(ANSI_RESET);
    return reset;
}

static_assert(sizeof(raw_ansi) == sizeof(uint16_t));

uint16_t raw_ansi::get_bits_raw() const
{
    uint16_t result;
    std::memcpy(&result, this, sizeof(uint16_t));
    return result;
}

uint16_t raw_ansi::get_bits_normalized() const
{
    auto tmp = *this;
    tmp.normalize();
    return tmp.get_bits_raw();
}

static bool self_test = []() -> bool {
    assert(raw_ansi().get_bits_raw() == 0);
    return true;
}();

bool raw_ansi::operator==(const raw_ansi &other) const
{
    return get_bits_normalized() == other.get_bits_normalized();
}

raw_ansi raw_ansi::difference(const raw_ansi &a, const raw_ansi &b)
{
    const uint16_t diff = a.get_bits_normalized() ^ b.get_bits_normalized();
    raw_ansi result;
    std::memcpy(&result, &diff, sizeof(uint16_t));
    return result;
}

AnsiString raw_ansi::getAnsiString(raw_ansi value)
{
    AnsiString output;
    value.normalize();

    if (value == raw_ansi()) {
        output.add_code(ANSI_RESET);
        return output;
    }

#define X(UPPER, lower, n) \
    if (value.lower) \
        output.add_code(ANSI_##UPPER);
    XFOREACH_ANSI_BIT(X)
#undef X

#define REPORT(x) \
    do { \
        if (value.x##_valid) \
            value.report_##x(output); \
    } while (false)

    REPORT(fg);
    REPORT(bg);

#undef REPORT

    return output;
}

AnsiString raw_ansi::transition(const raw_ansi &from, const raw_ansi &to)
{
    if (from == to) {
        /* nop */
        return AnsiString{};
    }

    if (from == raw_ansi() || to == raw_ansi()) {
        return to.asAnsiString();
    }

    const auto diff = difference(from, to);
    assert(diff.get_bits_raw() != 0);

#define SETS_BIT(bit) ((diff.bit != 0) && (to.bit != 0))
#define CLEARS_BIT(bit) ((diff.bit != 0) && (to.bit == 0))
#define BOTH_HAVE(bit) ((from.bit != 0) && (to.bit != 0))

    AnsiString diff_output;

#define X(UPPER, lower, n) \
    if (SETS_BIT(lower)) \
        diff_output.add_code(ANSI_##UPPER); \
    else if (CLEARS_BIT(lower)) \
        diff_output.add_code(ANSI_##UPPER##_OFF);
    XFOREACH_ANSI_BIT(X)
#undef X

#define REPORT(lower, UPPER) \
    do { \
        if (SETS_BIT(lower##_valid) \
            || (BOTH_HAVE(lower##_valid) && (diff.lower##_color != 0 || diff.lower##_hi != 0))) { \
            to.report_##lower(diff_output); \
        } else if (CLEARS_BIT(lower##_valid)) { \
            diff_output.add_code(ANSI_##UPPER##_DEFAULT); \
        } \
    } while (false)

    REPORT(fg, FG);
    REPORT(bg, BG);

#undef REPORT
#undef CLEARS_BIT
#undef SETS_BIT

    const AnsiString reset_output = to.asAnsiString().copy_as_reset();
    if (diff_output.size() < reset_output.size())
        return diff_output;
    else
        return reset_output;
}

const char *raw_ansi::describe(const int code)
{
    switch (code) {
    case -1:
        return "*error*";

    case ANSI_RESET:
        return "reset";

#define X(UPPER, lower, n) \
    case ANSI_##UPPER: \
        return #lower; \
    case ANSI_##UPPER##_OFF: \
        return #lower "-off";
        XFOREACH_ANSI_BIT(X)
#undef X

    case ANSI_FG_DEFAULT:
        return "default";

    case ANSI_BG_DEFAULT:
        return "on-default";

#define X(n, lower) \
    case ANSI_FG_COLOR + (n): \
        return #lower; \
    case ANSI_BG_COLOR + (n): \
        return "on-" #lower; \
    case ANSI_FG_COLOR_HI + (n): \
        return "bright-" #lower; \
    case ANSI_BG_COLOR_HI + (n): \
        return "on-bright-" #lower;
        XFOREACH_ANSI_COLOR(X)
#undef X
    default:
        break;
    }

    return "*unknown*";
}

#define REPORT(lower, UPPER) \
    void raw_ansi::report_##lower(AnsiString &output) const \
    { \
        assert(lower##_valid); \
        output.add_code(lower##_color \
                        + (lower##_hi ? ANSI_##UPPER##_COLOR_HI : ANSI_##UPPER##_COLOR)); \
    }

REPORT(fg, FG)
REPORT(bg, BG)

#undef REPORT

static uint16_t ansi_encode_color(const int n)
{
    assert((n & ~7) == 0);
    return static_cast<uint16_t>(n & 7);
}

static uint16_t ansi_encode_bit(const bool b)
{
    return static_cast<uint16_t>(b ? 1 : 0);
}

#define DEF_SET_CLEAR(x) \
    void raw_ansi::set_##x##_color(const int n, const bool hi) \
    { \
        x##_valid = 1; \
        x##_color = ansi_encode_color(n); \
        x##_hi = ansi_encode_bit(hi); \
    } \
    void raw_ansi::clear_##x##_color() \
    { \
        x##_valid = 0; \
        x##_color = 0; \
        x##_hi = 0; \
    }
DEF_SET_CLEAR(fg)
DEF_SET_CLEAR(bg)
#undef DEF_SET_CLEAR

void raw_ansi::normalize()
{
#define TRY_CLEAR(x) \
    do { \
        if (!x##_valid && (x##_color || x##_hi)) \
            clear_##x##_color(); \
    } while (false)
    TRY_CLEAR(fg);
    TRY_CLEAR(bg);
#undef TRY_CLEAR
}

void Ansi::process_code(const int code)
{
    switch (code) {
    case ANSI_RESET:
        ansi_ = raw_ansi();
        break;

#define X(UPPER, lower, n) \
    case ANSI_##UPPER: \
        ansi_.lower = 1; \
        break; \
    case ANSI_##UPPER##_OFF: \
        ansi_.lower = 0; \
        break;
        XFOREACH_ANSI_BIT(X)
#undef X

    case ANSI_FG_DEFAULT:
        ansi_.clear_fg_color();
        break;
    case ANSI_BG_DEFAULT:
        ansi_.clear_bg_color();
        break;

#define X(n, lower) \
    case ANSI_FG_COLOR + (n): \
        ansi_.set_fg_color((n), false); \
        break; \
    case ANSI_BG_COLOR + (n): \
        ansi_.set_bg_color((n), false); \
        break; \
    case ANSI_FG_COLOR_HI + (n): \
        ansi_.set_fg_color((n), true); \
        break; \
    case ANSI_BG_COLOR_HI + (n): \
        ansi_.set_bg_color((n), true); \
        break;
        XFOREACH_ANSI_COLOR(X)
#undef X

    default:
        break;
    }
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
static bool isValidAnsiCode(const int n)
{
    switch (n) {
    case ANSI_RESET:

#define X(UPPER, lower, n) \
    case ANSI_##UPPER: \
    case ANSI_##UPPER##_OFF:
        XFOREACH_ANSI_BIT(X)
#undef X

    case ANSI_FG_DEFAULT:
    case ANSI_BG_DEFAULT:

#define X(n, lower) \
    case ANSI_FG_COLOR + (n): \
    case ANSI_BG_COLOR + (n): \
    case ANSI_FG_COLOR_HI + (n): \
    case ANSI_BG_COLOR_HI + (n):
        XFOREACH_ANSI_COLOR(X)
#undef X
        return true;

    default:
        return false;
    }

    // Not supported by MUME, so they're not supported by MMapper:
    // 6: rapid blink
    // 8: conceal
    // 9: crossed-out
    // 10-19: alternate fonts
    // 28: reveal (conceal off)
    // 29: not crossed out
    // 38: set fg color; requires additional parsing: ";5;n" or "2;r;g;b"
    // 48: set bg color; requires additional parsing: ";5;n" or "2;r;g;b"
    // 51-55: encircled/overlined (rarely supported)
    // 60-65: ideograms (rarely supported)
}

bool isValidAnsiColor(const QStringRef &ansi)
{
    if (!isAnsiColor(ansi))
        return false;

    bool valid = true;
    ansiForeachColorCode(ansi, [&valid](int n) -> void {
        if (n < 0 || !isValidAnsiCode(n))
            valid = false;
    });
    return valid;
}

bool isValidAnsiColor(const QString &ansi)
{
    return isValidAnsiColor(ansi.midRef(0));
}

struct Prefix final
{
private:
    int prefixLen = 0;
    QStringRef quotePrefix;
    bool hasPrefix2 = false;
    int bulletLength = 0;
    QStringRef prefix2;
    bool valid_ = false;

public:
    int length() const { return prefixLen; }
    bool isValid() const { return valid_; }

    template<typename Appender>
    void write(Appender &&append) const
    {
        append(quotePrefix); // e.g. " > "
        if (hasPrefix2) {
            for (int i = 0; i < bulletLength; ++i)
                append(C_SPACE); // use " " instead of "*" after wrapping
            append(prefix2);     // whatever space came after the *
        }
    }

public:
    template<typename Appender>
    QStringRef init(QStringRef line, const int maxLen, Appender &&append)
    {
        // quotePrefix = prefix2 = line.left(0);

        // step 1: match the quoted prefix
        {
            auto m = quotePrefixRegex.match(line);
            if (m.hasMatch()) {
                const int len = m.capturedLength(0);
                quotePrefix = line.left(len);
                prefixLen = measureExpandedTabsOneLine(quotePrefix, 0);

                if (prefixLen >= maxLen) {
                    valid_ = false;
                    return line;
                }

                line = line.mid(len);
                append(quotePrefix);
            }
        }

        // step 2: See if there's a bullet. If so, we will only print it on the first line,
        // and we'll replace it with equivalent length whitespace on consecutive linewraps.
        {
            auto m = bulletPrefixRegex.match(line);
            if (m.hasMatch()) {
                const QStringRef &ref = m.capturedRef();
                /* this could fail if someone breaks the regex pattern for the escaped asterisk */
                bulletLength = ref.length();
                prefixLen = measureExpandedTabsOneLine(ref, prefixLen);
                hasPrefix2 = true;
                append(ref);
                line = line.mid(ref.length());
            }
        }

        // step 3: duplicate the exact whitespace following the bullet
        if (hasPrefix2) {
            auto m = leadingWhitespaceRegex.match(line);
            if (m.hasMatch()) {
                prefix2 = m.capturedRef();
                prefixLen = measureExpandedTabsOneLine(prefix2, prefixLen);
                line = line.mid(prefix2.length());
                append(prefix2);
            }
        }

        valid_ = true;
        return line;
    }
};

void TextBuffer::appendJustified(QStringRef line, const int maxLen)
{
    constexpr const bool preserveTrailingWhitespace = true;

    // REVISIT: consider ignoring the entire message if it contains ANSI!
    if (containsAnsi(line) || measureExpandedTabsOneLine(line, 0) <= maxLen) {
        append(line);
        return;
    }

    // note: Generic lambda is actually a template function.
    const auto appender = [this](auto &&arg) { this->append(std::forward<decltype(arg)>(arg)); };

    Prefix prefix;
    line = prefix.init(line, maxLen, appender);

    if (!prefix.isValid()) {
        append(line);
        return;
    }

    // wordwrapping between the prefixes and maxLength
    int col = prefix.length();
    while (!line.isEmpty()) {
        // identify any leading whitespace (there won't be on 1st pass)
        QStringRef leadingSpace = line.left(0);
        {
            auto m = leadingWhitespaceRegex.match(line);
            if (m.hasMatch()) {
                leadingSpace = m.capturedRef();
                line = line.mid(m.capturedLength());
            }
        }
        // find the next word, and see if leading whitespace plus the word
        // will result in a wrap. If so, print it. Otherwise, ignore the
        // leading whitespace, print a newline, the prefix(es), and then
        // print the word.
        {
            auto m = leadingNonSpaceRegex.match(line);
            if (m.hasMatch()) {
                line = line.mid(m.capturedLength());
                const auto word = m.capturedRef();
                const int spaceCol = measureExpandedTabsOneLine(leadingSpace, col);

                if (spaceCol + word.length() > maxLen) {
                    append('\n');
                    prefix.write(appender);
                    col = prefix.length();
                } else {
                    if ((false)) {
                        append(leadingSpace);
                        col = spaceCol;
                    } else if (!leadingSpace.isEmpty()) {
                        append(C_SPACE);
                        col += 1;
                    }
                }

                append(word);
                col += word.length();

            } else {
                assert(line.isEmpty());
                if ((preserveTrailingWhitespace))
                    append(leadingSpace);
            }
        }
    }
}

void TextBuffer::appendExpandedTabs(const QStringRef &line, const int start_at)
{
    int col = start_at;
    for (auto &c : line) {
        if (c == '\t') {
            const int spaces = 8 - (col % 8);
            col += spaces;
            for (int i = 0; i < spaces; ++i)
                append(' ');

        } else {
            col += 1;
            append(c);
        }
    }
}

void TextBuffer::appendWithoutTrailingWhitespace(QStringRef line)
{
    auto m = trailingWhitespaceRegex.match(line);
    if (m.hasMatch()) {
        line = line.mid(0, m.capturedStart());
    }
    append(line);
}

void TextBuffer::appendWithoutDuplicateSpaces(QStringRef line)
{
    /* Step 1: ignore any indendation the user applied */
    {
        auto m = leadingWhitespaceRegex.match(line);
        if (m.hasMatch()) {
            const int len = m.capturedLength();
            append(line.left(len));
            line = line.mid(len);
        }
    }
    /* Step 2: turn all other duplicate spaces into single spaces */
    {
        int pos = 0;
        auto it = twoOrMoreSpaceCharsRegex.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            const auto matchStart = m.capturedStart();
            if (matchStart > pos) {
                append(line.mid(pos, matchStart - pos));
            }
            append(' ');
            pos = m.capturedEnd();
        }
        if (pos != line.length())
            append(line.mid(pos, line.length() - pos));
    }
}

bool TextBuffer::isEmpty() const
{
    return text_.isEmpty();
}

bool TextBuffer::hasTrailingNewline() const
{
    return !isEmpty() && text_.at(text_.length() - 1) == '\n';
}

TextBuffer normalizeAnsi(const QStringRef &old)
{
    if (!containsAnsi(old)) {
        assert(false);
        TextBuffer output;
        output.append(old);
        return output;
    }

    TextBuffer output;
    output.reserve(2 * old.length()); /* no idea */

    const auto reset = AnsiString::get_reset_string();

    Ansi ansi;

    foreachLine(old, [&ansi, &reset, &output](const QStringRef &line, bool hasNewline) {
        if (ansi != Ansi())
            output.append(ansi.get_raw().asAnsiString().c_str());

        Ansi current = ansi;
        Ansi next = ansi;

        const auto transition = [&current, &next, &output]() {
            if (current == next)
                return;
            AnsiString delta = raw_ansi::transition(current.get_raw(), next.get_raw());
            output.append(delta.c_str());
            current = next;
        };

        const auto print = [&transition, &output](const QStringRef &ref) {
            transition();
            output.append(ref);
        };

        int pos = 0;
        foreachAnsi(line, [&next, &line, &pos, &print](const int begin, const QStringRef &ansiStr) {
            assert(line.at(begin) == '\x1b');
            if (begin > pos) {
                print(line.mid(pos, begin - pos));
            }

            pos = begin + ansiStr.length();
            ansiForeachColorCode(ansiStr, [&next](int code) { next.process_code(code); });
        });

        if (pos < line.length()) {
            print(line.mid(pos));
        }

        if (current != next || next != Ansi()) {
            output.append(reset.c_str());
        }

        if (hasNewline)
            output.append('\n');
        ansi = next;
    });

    return output;
}

TextBuffer normalizeAnsi(const QString &str)
{
    return normalizeAnsi(str.midRef(0));
}

AnsiStringToken::AnsiStringToken(AnsiStringToken::TokenTypeEnum _type,
                                 const QString &_text,
                                 const AnsiStringToken::size_type _offset,
                                 const AnsiStringToken::size_type _length)
    : type{_type}
    , text_{&_text}
    , offset_{_offset}
    , length_{_length}
{
    const auto maxlen = _text.length();
    assert(isClamped(start_offset(), 0, maxlen));
    assert(isClamped(length(), 0, maxlen));
    assert(isClamped(end_offset(), 0, maxlen));
}

QStringRef AnsiStringToken::getQStringRef() const
{
    return QStringRef{text_, offset_, length_};
}

bool AnsiStringToken::isAnsiCsi() const
{
    return type == TokenTypeEnum::ANSI && length() >= 4 && at(0) == QC_ESC
           && at(1) == QC_OPEN_BRACKET && at(length() - 1) == 'm';
}

AnsiTokenizer::Iterator::Iterator(const QString &_str, AnsiTokenizer::Iterator::size_type _pos)
    : str_{_str}
    , pos_{_pos}
{
    assert(isClamped(pos_, 0, str_.size()));
}

AnsiStringToken AnsiTokenizer::Iterator::next()
{
    assert(hasNext());
    const auto len = str_.size();
    const auto token = getCurrent();
    assert(isClamped(pos_, 0, len));
    assert(token.start_offset() == pos_);
    assert(isClamped(token.length(), 1, len - pos_));
    pos_ = token.end_offset();
    return token;
}

AnsiStringToken AnsiTokenizer::Iterator::getCurrent()
{
    const auto here = pos_;
    const QChar &c = str_[here];
    if (c == QC_ESC) {
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::ANSI, str_, here, skip_ansi()};
    } else if (c == QC_NEWLINE) {
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::NEWLINE, str_, here, 1};
    }
    // Special case to match "\r\n" as just "\n"
    else if (c == QC_CARRIAGE_RETURN && here + 1 < str_.size() && str_[here + 1] == QC_NEWLINE) {
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::NEWLINE, str_, here + 1, 1};
    } else if (c == QC_CARRIAGE_RETURN || isControl(c)) {
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::CONTROL, str_, here, skip_control()};
    } else if (c.isSpace() && c != QC_NBSP) {
        // TODO: Find out of this includes control codes like form-feed ('\f') and vertical-tab ('\v').
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::SPACE, str_, here, skip_space()};
    } else {
        return AnsiStringToken{AnsiStringToken::TokenTypeEnum::WORD, str_, here, skip_word()};
    }
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_ansi()
{
    // hack to avoid having to have two separate stop return values
    // (one to stop before current value, and one to include it)
    bool sawLetter = false;
    return skip([&sawLetter](const QChar &c) -> ResultEnum {
        if (sawLetter || c == QC_ESC || c == QC_NBSP || c.isSpace())
            return ResultEnum::STOP;

        if (c == QC_OPEN_BRACKET || c == QC_SEMICOLON || c.isDigit())
            return ResultEnum::KEEPGOING;

        if (c.isLetter()) {
            /* include the letter, but stop afterwards */
            sawLetter = true;
            return ResultEnum::KEEPGOING;
        }

        /* ill-formed ansi code */
        return ResultEnum::STOP;
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_control()
{
    return skip([](const QChar &c) -> ResultEnum {
        return isControl(c) ? ResultEnum::KEEPGOING : ResultEnum::STOP;
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_space()
{
    return skip([](const QChar &c) -> ResultEnum {
        switch (c.toLatin1()) {
        case C_ESC:
        case C_NBSP:
        case C_CARRIAGE_RETURN:
        case C_NEWLINE:
            return ResultEnum::STOP;
        default:
            return c.isSpace() ? ResultEnum::KEEPGOING : ResultEnum::STOP;
        }
    });
}

AnsiTokenizer::Iterator::size_type AnsiTokenizer::Iterator::skip_word()
{
    return skip([](const QChar &c) -> ResultEnum {
        switch (c.toLatin1()) {
        case C_ESC:
        case C_NBSP:
        case C_CARRIAGE_RETURN:
        case C_NEWLINE:
            return ResultEnum::STOP;
        default:
            return (c.isSpace() || isControl(c)) ? ResultEnum::STOP : ResultEnum::KEEPGOING;
        }
    });
}

char toLowerLatin1(const char c)
{
    const int i = static_cast<int>(static_cast<uint8_t>(c));
    if (i >= 192 && i <= 221 && i != 215) {
        // handle accented Latin-1 chars
        return static_cast<char>(i + 32);
    }
    return static_cast<char>(std::tolower(c));
}

std::string toLowerLatin1(const std::string_view &str)
{
    std::ostringstream oss;
    for (char c : str) {
        oss << toLowerLatin1(c);
    }
    return oss.str();
}

bool isAbbrev(const std::string_view &abbr, const std::string_view &fullText)
{
    return !abbr.empty()                               //
           && abbr.size() <= fullText.size()           //
           && abbr == fullText.substr(0, abbr.size()); //
}

// std::isprint is only ASCII
bool isPrintLatin1(char c)
{
    const auto uc = static_cast<uint8_t>(c);
    return uc >= ((uc < 0x7f) ? 0x20 : 0xa0);
}

bool requiresQuote(const std::string_view &str)
{
    for (auto &c : str) {
        if (std::isspace(c) || !isPrintLatin1(c))
            return true;
    }
    return false;
}

std::ostream &print_char(std::ostream &os, char c, bool doubleQuote)
{
    switch (c) {
    case C_ESC:
        os << "\\e"; // Not valid C++, but borrowed from /bin/echo.
        break;
    case '\a':
        os << "\\a";
        break;
    case '\b':
        os << "\\b";
        break;
    case '\f':
        os << "\\f";
        break;
    case '\n':
        os << "\\n";
        break;
    case '\r':
        os << "\\r";
        break;
    case '\t':
        os << "\\t";
        break;
    case '\v':
        os << "\\v";
        break;
    case '\\':
        os << "\\\\";
        break;
    case '\0':
        // NOTE: If we were generating C++, this could emit incorrect results (e.g. '0' followed by '0'),
        // but this format only allows octal values with '\o###'.
        os << "\\0";
        break;
    default:
        if (isPrintLatin1(c)) {
            if (c == (doubleQuote ? '"' : '\''))
                os << '\\';
            os << c;
        } else {
            // NOTE: This form can generate invalid C++.
            os << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (c & 0xff) << std::dec
               << std::setfill(' ');
        }
        break;
    }
    return os;
}

std::ostream &print_char_quoted(std::ostream &os, const char c)
{
    static constexpr const char C_SQUOTE = '\'';
    os << C_SQUOTE;
    print_char(os, c, false);
    return os << C_SQUOTE;
}

std::ostream &print_string_quoted(std::ostream &os, const std::string_view &sv)
{
    static constexpr const char C_DQUOTE = '"';
    os << C_DQUOTE;
    for (const auto &c : sv) {
        print_char(os, c, true);
    }
    return os << C_DQUOTE;
}

std::ostream &print_string_smartquote(std::ostream &os, const std::string_view &sv)
{
    if (!requiresQuote(sv)) {
        return os << sv;
    }
    return print_string_quoted(os, sv);
}

QString toQStringLatin1(const std::string_view &sv)
{
    return QString::fromLatin1(sv.data(), static_cast<int>(sv.size()));
}

QString toQStringUtf8(const std::string_view &sv)
{
    return QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
}

QByteArray toQByteArrayLatin1(const std::string_view &sv)
{
    return QByteArray(sv.data(), static_cast<int>(sv.size()));
}

QByteArray toQByteArrayUtf8(const std::string_view &sv)
{
    return toQStringUtf8(sv).toUtf8();
}

std::string toStdStringLatin1(const QString &qs)
{
    return qs.toLatin1().toStdString();
}

std::string toStdStringUtf8(const QString &qs)
{
    return qs.toUtf8().toStdString();
}
