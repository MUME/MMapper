#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "Consts.h"
#include "utils.h"

#include <cassert>
#include <cctype>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <QRegularExpression>
#include <QString>

namespace mmqt {
NODISCARD bool containsAnsi(QStringView str);
NODISCARD bool containsAnsi(const QString &str);
} // namespace mmqt

// Callback = void(string_view);
// callback is either a span excluding c, or a span of contiguous c's.
template<typename Callback>
void foreachChar(const std::string_view input, const char c, Callback &&callback)
{
    std::string_view sv = input;
    while (!sv.empty()) {
        if (const auto next = sv.find(c); next == std::string_view::npos) {
            callback(sv);
            break;
        } else if (next > 0) {
            callback(sv.substr(0, next));
            sv.remove_prefix(next);
        }
        assert(!sv.empty() && sv.front() == c);
        if (const auto span = sv.find_first_not_of(c); span == std::string_view::npos) {
            callback(sv);
            break;
        } else {
            callback(sv.substr(0, span));
            sv.remove_prefix(span);
        }
    }
}

// Callback = void(string_view);
template<typename Callback>
void foreachLine(const std::string_view input, Callback &&callback)
{
    using char_consts::C_NEWLINE;

    const size_t len = input.size();
    size_t pos = 0;
    while (pos < len) {
        const auto next = input.find(C_NEWLINE, pos);
        if (next == std::string_view::npos)
            break;
        assert(next >= pos);
        assert(input[next] == C_NEWLINE);
        callback(input.substr(pos, next - pos + 1));
        pos = next + 1;
    }
    if (pos < len)
        callback(input.substr(pos, len - pos));
}

namespace mmqt {
// Callback is void(int pos)
template<typename Callback>
void foreachChar(const QStringView input, char c, Callback &&callback)
{
    const auto len = input.size();
    int pos = 0;
    while (pos < len) {
        const auto next = input.indexOf(c, pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == c);
        callback(next);
        pos = static_cast<int>(next + 1);
    }
}

template<typename Callback>
inline void foreachChar(const QString &input, const char c, Callback &&callback)
{
    foreachChar(QStringView{input}, c, std::forward<Callback>(callback));
}

// Callback can be:
// void(const QStringView line, bool hasNewline), or
// void(QStringView line, bool hasNewline)
template<typename Callback>
void foreachLine(const QStringView input, Callback &&callback)
{
    using char_consts::C_NEWLINE;

    const auto len = input.size();
    int pos = 0;
    while (pos < len) {
        const auto next = input.indexOf(C_NEWLINE, pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == C_NEWLINE);
        callback(input.mid(pos, next - pos), true);
        pos = static_cast<int>(next + 1);
    }
    if (pos < len)
        callback(input.mid(pos, len - pos), false);
}

template<typename Callback>
inline void foreachLine(const QString &input, Callback &&callback)
{
    foreachLine(QStringView{input}, std::forward<Callback>(callback));
}

extern const QRegularExpression weakAnsiRegex;

// Reports any potential ANSI sequence, including invalid sequences.
// Use isAnsiColor(ref) to verify if the value reported is a color.
//
// Callback:
// void(int start, QStringView sv)
//
// NOTE: This version only reports callback(start, length),
// because the intended caller needs the start position,
// but QStringView can't expose the start position.
template<typename Callback>
void foreachAnsi(const QStringView line, Callback &&callback)
{
    const auto len = line.size();
    int pos = 0;
    while (pos < len) {
        auto m = weakAnsiRegex.match(line, pos);
        if (!m.hasMatch())
            break;
        callback(m.capturedStart(), m.capturedView());
        pos = m.capturedEnd();
    }
}

template<typename Callback>
void foreachAnsi(const QString &line, Callback &&callback)
{
    foreachAnsi(QStringView{line}, std::forward<Callback>(callback));
}

NODISCARD extern bool isAnsiColor(QStringView ansi);
NODISCARD extern bool isAnsiColor(const QString &ansi);

class NODISCARD AnsiColorParser final
{
private:
    void *const data_;
    void (*const callback_)(void *data, int);
    explicit AnsiColorParser(void *const _data, void (*const _callback)(void *data, int))
        : data_{_data}
        , callback_{_callback}
    {}

    void for_each(QStringView ansi) const;
    void report(int n) const { callback_(data_, n); }

public:
    // "ESC[m"     -> callback(0)
    // "ESC[0m"    -> callback(0)
    // "ESC[1;33m" -> callback(1); callback(33)
    //
    // NOTE: Empty values other than "ESC[m" are errors.
    // e.g. ESC[;;m -> callback(-1); callback(-1); callback(-1)
    //
    // NOTE: Values too large to fit in a signed integer
    // (e.g. "ESC[2147483648m" -> callback(-1).
    template<typename Callback>
    static void for_each_code(const QStringView ansi, Callback &&callback)
    {
        auto ptr = &callback;
        using ptr_type = decltype(ptr);

        /* use type-erasure to avoid putting implementation in the header */
        const auto type_erased = [](void *data, int n) -> void {
            Callback &cb = *reinterpret_cast<ptr_type>(data);
            cb(n);
        };
        AnsiColorParser{reinterpret_cast<void *>(ptr), type_erased}.for_each(ansi);
    }
};

// Input must be a valid ansi color code: ESC[...m,
// where the ... is any number of digits or semicolons.
// Callback:
// void(int csi)
template<typename Callback>
void ansiForeachColorCode(const QStringView ansi, Callback &&callback)
{
    AnsiColorParser::for_each_code(ansi, std::forward<Callback>(callback));
}

template<typename Callback>
void ansiForeachColorCode(const QString &ansi, Callback &&callback)
{
    ansiForeachColorCode(QStringView{ansi}, std::forward<Callback>(callback));
}

NODISCARD extern bool isValidAnsiColor(QStringView ansi);
NODISCARD extern bool isValidAnsiColor(const QString &ansi);

NODISCARD extern int countLines(const QString &input);
NODISCARD extern int countLines(QStringView input);

NODISCARD extern int measureExpandedTabsOneLine(QStringView line, int starting_at);
NODISCARD extern int measureExpandedTabsOneLine(const QString &line, int starting_at);

NODISCARD extern int findTrailingWhitespace(QStringView line);
NODISCARD extern int findTrailingWhitespace(const QString &line);

class NODISCARD TextBuffer final
{
private:
    QString text_;

public:
    void reserve(int len) { text_.reserve(len); }
    NODISCARD const QString &getQString() const { return text_; }
    NODISCARD auto length() const { return text_.length(); }

public:
    void append(char c) { text_.append(c); }
    void append(QChar c) { text_.append(c); }
    void append(const QString &line) { text_.append(line); }
    void append(const QStringView line) { text_.append(line); }

public:
    void appendJustified(QStringView line, int maxLen);
    void appendExpandedTabs(QStringView line, int start_at = 0);

public:
    NODISCARD bool isEmpty() const;
    NODISCARD bool hasTrailingNewline() const;
};
} // namespace mmqt

class NODISCARD AnsiString final
{
private:
    std::string buffer_;

public:
    AnsiString();
    void add_code(int code);
    NODISCARD AnsiString copy_as_reset() const;

public:
    NODISCARD inline bool isEmpty() const { return buffer_.empty(); }
    NODISCARD inline int size() const { return static_cast<int>(buffer_.size()); }
    NODISCARD const char *c_str() const { return buffer_.c_str(); }

public:
    NODISCARD static AnsiString get_reset_string();
};

#define XFOREACH_ANSI_BIT(X) \
    X(BOLD, bold, 1) \
    X(FAINT, faint, 2) \
    X(ITALIC, italic, 3) \
    X(UNDERLINE, underline, 4) \
    X(BLINK, blink, 5) \
    X(REVERSE, reverse, 7)

struct NODISCARD raw_ansi final
{
#define X(UPPER, lower, n) uint16_t lower : 1;
    XFOREACH_ANSI_BIT(X)
#undef X
    uint16_t fg_valid : 1;
    uint16_t fg_color : 3;
    uint16_t fg_hi : 1;
    uint16_t bg_valid : 1;
    uint16_t bg_color : 3;
    uint16_t bg_hi : 1;

    constexpr raw_ansi()
        :
#define X(UPPER, lower, n) lower(0),
        XFOREACH_ANSI_BIT(X)
#undef X
            fg_valid(0)
        , fg_color(0)
        , fg_hi(0)
        , bg_valid(0)
        , bg_color(0)
        , bg_hi(0)
    {}

    void set_fg_color(int n, bool hi);
    void set_bg_color(int n, bool hi);
    void clear_fg_color();
    void clear_bg_color();

    void normalize();

    NODISCARD uint16_t get_bits_raw() const;
    NODISCARD uint16_t get_bits_normalized() const;

    bool operator==(const raw_ansi &other) const;
    bool operator!=(const raw_ansi &other) const
    {
        return !operator==(other);
    }
    NODISCARD static raw_ansi difference(const raw_ansi &a, const raw_ansi &b);

    NODISCARD AnsiString asAnsiString() const
    {
        return getAnsiString(*this);
    }
    NODISCARD static AnsiString getAnsiString(raw_ansi value);
    NODISCARD static AnsiString transition(const raw_ansi &from, const raw_ansi &to);

    NODISCARD static const char *describe(int code);

private:
    void report_fg(AnsiString &output) const;
    void report_bg(AnsiString &output) const;
};

class NODISCARD Ansi final
{
private:
    raw_ansi ansi_;

public:
    void process_code(int code);
    NODISCARD inline raw_ansi get_raw() const { return ansi_; }
    bool operator==(const Ansi &other) const { return ansi_ == other.ansi_; }
    bool operator!=(const Ansi &other) const { return ansi_ != other.ansi_; }
};

namespace mmqt {
/**
 * NOTE:
 * 1. Code will assert() if the text does not contain any ansi strings.
 * 2. Input is assumed to start with reset ansi (ESC[0m).
 * 3. All ansi codes will be normalized (e.g. ESC[1mESC[32m -> ESC[1;32m).
 * 4. Ansi will be explicitly reset before every newline, and it will
 *    be restored at the start of the next line.
 * 5. The function assumes it's the entire document, and it always resets
 *    the ansi color at the end, even if there's no newline. Therefore,
 *    the function doesn't return a "current" ansi color.
 * 6. Assumes UNIX-style newlines (LF only, not CRLF).
 */
NODISCARD TextBuffer normalizeAnsi(QStringView);
NODISCARD TextBuffer normalizeAnsi(const QString &str);

class NODISCARD AnsiStringToken final
{
public:
    using size_type = decltype(std::declval<QString>().size());

    enum class NODISCARD TokenTypeEnum { ANSI, CONTROL, NEWLINE, SPACE, WORD };
    TokenTypeEnum type = TokenTypeEnum::ANSI; // There is no good default value.

private:
    const QStringView text_;
    const size_type offset_;
    const size_type length_;

public:
    explicit AnsiStringToken(TokenTypeEnum _type,
                             const QString &_text,
                             size_type _offset,
                             size_type _length);
    NODISCARD size_type length() const { return length_; }
    NODISCARD size_type start_offset() const { return offset_; }
    NODISCARD size_type end_offset() const { return offset_ + length_; }

public:
    NODISCARD QChar at(const size_type pos) const
    {
        assert(isClamped(pos, 0, length_));
        return text_.at(offset_ + pos);
    }
    NODISCARD QChar operator[](const size_type pos) const { return at(pos); }

public:
    NODISCARD auto begin() const { return text_.begin() + offset_; }
    NODISCARD auto end() const { return begin() + length_; }

public:
    NODISCARD QStringView getQStringView() const;
    NODISCARD bool isAnsiCsi() const;
};

struct NODISCARD AnsiTokenizer final
{
    class NODISCARD Iterator final
    {
    public:
        using size_type = AnsiStringToken::size_type;

    private:
        const QString &str_;
        size_type pos_;

    public:
        explicit Iterator(const QString &_str, size_type _pos = 0);

        bool operator!=(std::nullptr_t) const { return hasNext(); }
        bool operator!=(const Iterator &other) const = delete;

        /*
         * NOTE: The `operator*()` and `operator++()` paradigm is subtly
         * different from the `while (hasNext()) foo(next());` paradigm.
         *
         * With a range-based for loop,
         *   for (YourType yourVarName : yourRange) {
         *     // your code here
         *   }
         * becomes:
         *   auto _it = std::begin(yourRange); // defaults to yourRange.begin()
         *   auto _end = std::end(yourRange);  // defaults to yourRange.end()
         *   while (_it != _end) {             // calls _it.operator!=(_end)
         *     YourType yourVarName = *_it;    // calls _it.operator*()
         *     // your code here
         *     ++_it;                          // calls _it.operator++()
         *   }
         *
         * The current implementation is poorly optimized for range-based
         * for-loops because it will end up calling `getCurrent()` in both
         * `operator*()` and `operator++()`. This could be fixed by storing
         * an integer offset when it's computed in `operator*()`,
         * and then using that value in `operator++`.
         *
         * It would make iterator copies marginally slower, but it would
         * end up being faster in most cases. If you do that, consider
         * using a boolean value to make it equivalent to std::optional<T>,
         * (use negative values for that purpose).
         */
        AnsiStringToken operator*() { return getCurrent(); }
        Iterator &operator++()
        {
            MAYBE_UNUSED const auto ignored = next();
            return *this;
        }

        NODISCARD bool hasNext() const { return pos_ < str_.size(); }
        NODISCARD AnsiStringToken next();
        NODISCARD AnsiStringToken getCurrent();

    private:
        enum class NODISCARD ResultEnum { KEEPGOING, STOP };

        template<typename Callback>
        size_type skip(Callback &&check)
        {
            const auto len = str_.size();
            const auto start = pos_;
            assert(isClamped(start, 0, len));
            auto it = start + 1;
            for (; it < len; ++it)
                if (check(str_[it]) == ResultEnum::STOP)
                    break;
            assert(isClamped(it, start, len));
            return it - start;
        }
        NODISCARD size_type skip_ansi();
        NODISCARD static bool isControl(const QChar c)
        {
            return std::iscntrl(c.toLatin1()) && c != QC_NBSP;
        }
        NODISCARD size_type skip_control();
        NODISCARD size_type skip_space();
        NODISCARD size_type skip_word();
    };

    const QString &str_;
    AnsiTokenizer(QString &&) = delete;
    explicit AnsiTokenizer(const QString &_str)
        : str_{_str}
    {}

    NODISCARD Iterator begin() { return Iterator{str_, 0}; }
    NODISCARD auto end() { return nullptr; }
};
} // namespace mmqt

NODISCARD static inline constexpr int tab_advance(int col)
{
    return 8 - (col % 8);
}

NODISCARD static inline constexpr int next_tab_stop(int col)
{
    return col + tab_advance(col);
}
static_assert(next_tab_stop(0) == 8);
static_assert(next_tab_stop(1) == 8);
static_assert(next_tab_stop(7) == 8);
static_assert(next_tab_stop(8) == 16);
static_assert(next_tab_stop(9) == 16);
static_assert(next_tab_stop(15) == 16);

NODISCARD extern char toLowerLatin1(char c);
NODISCARD extern std::string toLowerLatin1(std::string_view str);
NODISCARD extern char toUpperLatin1(char c);
NODISCARD extern std::string toUpperLatin1(std::string_view str);
NODISCARD extern bool isAbbrev(std::string_view abbr, std::string_view fullText);
NODISCARD extern bool isPrintLatin1(char c);
NODISCARD extern bool requiresQuote(std::string_view str);
std::ostream &print_char(std::ostream &os, char c, bool doubleQuote);
std::ostream &print_char_quoted(std::ostream &os, char c);
std::ostream &print_string_quoted(std::ostream &os, std::string_view sv);
std::ostream &print_string_smartquote(std::ostream &os, std::string_view sv);

struct NODISCARD QuotedChar final
{
private:
    char m_c = char_consts::C_NUL;

public:
    explicit QuotedChar(char c)
        : m_c(c)
    {}

    friend std::ostream &operator<<(std::ostream &os, const QuotedChar &quotedChar)
    {
        return print_char_quoted(os, quotedChar.m_c);
    }
};

// Use this instead of std::quoted()
// NOTE: The reason there's no QuotedStringView is because it's not possible to guard against xvalues.
struct NODISCARD QuotedString final
{
private:
    std::string m_str;

public:
    explicit QuotedString(std::string s)
        : m_str(std::move(s))
    {}

    friend std::ostream &operator<<(std::ostream &os, const QuotedString &quotedString)
    {
        return print_string_quoted(os, quotedString.m_str);
    }
};

struct NODISCARD SmartQuotedString final
{
private:
    std::string m_str;

public:
    explicit SmartQuotedString(std::string s)
        : m_str(std::move(s))
    {}

    friend std::ostream &operator<<(std::ostream &os, const SmartQuotedString &smartQuotedString)
    {
        return print_string_smartquote(os, smartQuotedString.m_str);
    }
};

namespace mmqt {
NODISCARD extern QString toQStringLatin1(std::string_view sv);
NODISCARD extern QString toQStringUtf8(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayLatin1(std::string_view sv);
NODISCARD extern QByteArray toQByteArrayUtf8(std::string_view sv);
NODISCARD extern std::string toStdStringLatin1(const QString &qs);
NODISCARD extern std::string toStdStringUtf8(const QString &qs);
NODISCARD extern std::string_view toStdStringViewLatin1(const QByteArray &arr);
NODISCARD extern std::string_view toStdStringViewLatin1(const QByteArray &&) = delete;
} // namespace mmqt
