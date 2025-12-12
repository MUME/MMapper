// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "TextBuffer.h"

#include "AnsiTextUtils.h"
#include "TabUtils.h"

using namespace char_consts;

namespace mmqt {

// allows ">" or "|" as the quote character
static const QRegularExpression quotePrefixRegex(R"(^[[:space:]]*([>|][[:space:]]*)*)");

// literal "*" or "-" bullets
// numbered list "1." or "1)" .. "99." or "99)" (have to draw the line somewhere).
// lettered list "a." or "a)"
static const QRegularExpression bulletPrefixRegex(
    R"(^([*]|[-]|[[:alnum:]][.)]|[[1-9][[:digit:]][.)]))");
static const QRegularExpression leadingWhitespaceRegex(R"(^[[:space:]]+)");
static const QRegularExpression leadingNonSpaceRegex(R"(^[^[:space:]]+)");

} // namespace mmqt

namespace mmqt {

struct NODISCARD Prefix final
{
private:
    int prefixLen = 0;
    QString quotePrefix;
    bool hasPrefix2 = false;
    qsizetype bulletLength = 0;
    QString prefix2;
    bool valid_ = false;

public:
    NODISCARD int length() const { return prefixLen; }
    NODISCARD bool isValid() const { return valid_; }

    template<typename Appender>
    void write(Appender &&append) const
    {
        append(quotePrefix); // e.g. " > "
        if (hasPrefix2) {
            for (int i = 0; i < bulletLength; ++i) {
                append(C_SPACE); // use " " instead of "*" after wrapping
            }
            append(prefix2); // whatever space came after the *
        }
    }

public:
    template<typename Appender>
    NODISCARD QString init(QString line, const int maxLen, Appender &&append)
    {
        // quotePrefix = prefix2 = line.left(0);

        // step 1: match the quoted prefix
        {
            auto m = quotePrefixRegex.match(line);
            if (m.hasMatch()) {
                const auto len = m.capturedLength(0);
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
                const QString sv = m.captured();
                /* this could fail if someone breaks the regex pattern for the escaped asterisk */
                bulletLength = sv.length();
                prefixLen = measureExpandedTabsOneLine(sv, prefixLen);
                hasPrefix2 = true;
                append(sv);
                line = line.mid(sv.length());
            }
        }

        // step 3: duplicate the exact whitespace following the bullet
        if (hasPrefix2) {
            auto m = leadingWhitespaceRegex.match(line);
            if (m.hasMatch()) {
                prefix2 = m.captured();
                prefixLen = measureExpandedTabsOneLine(prefix2, prefixLen);
                line = line.mid(prefix2.length());
                append(prefix2);
            }
        }

        valid_ = true;
        return line;
    }
};

void TextBuffer::appendJustified(const QStringView input_line, const int maxLen)
{
    QString line = input_line.toString();
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
        QString leadingSpace = line.left(0);
        {
            auto m = leadingWhitespaceRegex.match(line);
            if (m.hasMatch()) {
                leadingSpace = m.captured();
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
                const auto word = m.captured();
                const int spaceCol = measureExpandedTabsOneLine(leadingSpace, col);

                if (spaceCol + word.length() > maxLen) {
                    append(C_NEWLINE);
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
                col += static_cast<int>(word.length());

            } else {
                assert(line.isEmpty());
                if ((preserveTrailingWhitespace)) {
                    append(leadingSpace);
                }
            }
        }
    }
}

void TextBuffer::appendExpandedTabs(const QStringView line, const int start_at)
{
    int col = start_at;
    for (const QChar c : line) {
        if (c == C_TAB) {
            const int spaces = 8 - (col % 8);
            col += spaces;
            for (int i = 0; i < spaces; ++i) {
                append(C_SPACE);
            }

        } else {
            col += 1;
            append(c);
        }
    }
}

bool TextBuffer::isEmpty() const
{
    return m_text.isEmpty();
}

bool TextBuffer::hasTrailingNewline() const
{
    return !isEmpty() && m_text.back() == char_consts::C_NEWLINE;
}

} // namespace mmqt
