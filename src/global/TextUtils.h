#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cassert>
#include <cctype>
#include <optional>
#include <string>
#include <QRegularExpression>
#include <QString>

#include "utils.h"

bool containsAnsi(const QStringRef &str);
bool containsAnsi(const QString &str);

// Callback is void(int pos)
template<typename Callback>
void foreachChar(const QStringRef &input, char c, Callback &&callback)
{
    const int len = input.size();
    int pos = 0;
    while (pos < len) {
        const int next = input.indexOf(c, pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == c);
        callback(next);
        pos = next + 1;
    }
}

template<typename Callback>
inline void foreachChar(const QString &input, const char c, Callback &&callback)
{
    foreachChar(input.midRef(0), c, std::forward<Callback>(callback));
}

// Callback can be:
// void(const QStringRef& line, bool hasNewline), or
// void(QStringRef line, bool hasNewline)
template<typename Callback>
void foreachLine(const QStringRef &input, Callback &&callback)
{
    const int len = input.size();
    int pos = 0;
    while (pos < len) {
        const int next = input.indexOf('\n', pos);
        if (next < 0)
            break;
        assert(next >= pos);
        assert(input[next] == '\n');
        callback(input.mid(pos, next - pos), true);
        pos = next + 1;
    }
    if (pos < len)
        callback(input.mid(pos, len - pos), false);
}

template<typename Callback>
inline void foreachLine(const QString &input, Callback &&callback)
{
    foreachLine(input.midRef(0), std::forward<Callback>(callback));
}

extern const QRegularExpression weakAnsiRegex;

// Reports any potential ANSI sequence, including invalid sequences.
// Use isAnsiColor(ref) to verify if the value reported is a color.
//
// Callback:
// void(int start, QStringRef ref)
//
// NOTE: This version only reports callback(start, length),
// because the intended caller needs the start position,
// but QStringRef can't expose the start position.
template<typename Callback>
void foreachAnsi(const QStringRef &line, Callback &&callback)
{
    const int len = line.size();
    int pos = 0;
    while (pos < len) {
        auto m = weakAnsiRegex.match(line, pos);
        if (!m.hasMatch())
            break;
        callback(m.capturedStart(), m.capturedRef());
        pos = m.capturedEnd();
    }
}

template<typename Callback>
void foreachAnsi(const QString &line, Callback &&callback)
{
    foreachAnsi(line.midRef(0), std::forward<Callback>(callback));
}

bool isAnsiColor(QStringRef ansi);
bool isAnsiColor(const QString &ansi);

class AnsiColorParser final
{
private:
    void *const data_;
    void (*const callback_)(void *data, int);
    explicit AnsiColorParser(void *const _data, void (*const _callback)(void *data, int))
        : data_{_data}
        , callback_{_callback}
    {}

    void for_each(QStringRef ansi) const;
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
    static void for_each_code(const QStringRef &ansi, Callback &&callback)
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
void ansiForeachColorCode(const QStringRef &ansi, Callback &&callback)
{
    AnsiColorParser::for_each_code(ansi, std::forward<Callback>(callback));
}

template<typename Callback>
void ansiForeachColorCode(const QString &ansi, Callback &&callback)
{
    ansiForeachColorCode(ansi.midRef(0), std::forward<Callback>(callback));
}

bool isValidAnsiColor(const QStringRef &ansi);
bool isValidAnsiColor(const QString &ansi);

int countLines(const QString &input);
int countLines(const QStringRef &input);

int measureExpandedTabsOneLine(const QStringRef &line, int starting_at);
int measureExpandedTabsOneLine(const QString &line, int starting_at);
int measureExpandedTabsMultiline(const QStringRef &old);
int measureExpandedTabsMultiline(const QString &old);

int findTrailingWhitespace(const QStringRef &line);
int findTrailingWhitespace(const QString &line);

class TextBuffer final
{
private:
    QString text_;

public:
    void reserve(int len) { text_.reserve(len); }
    const QString &getQString() const { return text_; }
    auto length() const { return text_.length(); }

public:
    void append(char c) { text_.append(c); }
    void append(QChar c) { text_.append(c); }
    void append(const QString &line) { text_.append(line); }
    void append(const QStringRef &line) { text_.append(line); }

public:
    void appendJustified(QStringRef line, int maxLen);
    void appendExpandedTabs(const QStringRef &line, int start_at = 0);
    void appendWithoutTrailingWhitespace(QStringRef line);
    void appendWithoutDuplicateSpaces(QStringRef line);

public:
    bool isEmpty() const;
    bool hasTrailingNewline() const;
};

class AnsiString final
{
private:
    std::string buffer_;

public:
    AnsiString();
    void add_code(int code);
    AnsiString copy_as_reset() const;

public:
    inline bool isEmpty() const { return buffer_.empty(); }
    inline int size() const { return static_cast<int>(buffer_.size()); }
    const char *c_str() const { return buffer_.c_str(); }

public:
    static AnsiString get_reset_string();
};

#define XFOREACH_ANSI_BIT(X) \
    X(BOLD, bold, 1) \
    X(FAINT, faint, 2) \
    X(ITALIC, italic, 3) \
    X(UNDERLINE, underline, 4) \
    X(BLINK, blink, 5) \
    X(REVERSE, reverse, 7)

struct raw_ansi final
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

    uint16_t get_bits_raw() const;
    uint16_t get_bits_normalized() const;

    bool operator==(const raw_ansi &other) const;
    bool operator!=(const raw_ansi &other) const { return !operator==(other); }
    static raw_ansi difference(const raw_ansi &a, const raw_ansi &b);

    AnsiString asAnsiString() const { return getAnsiString(*this); }
    static AnsiString getAnsiString(raw_ansi value);
    static AnsiString transition(const raw_ansi &from, const raw_ansi &to);

    static const char *describe(int code);

private:
    void report_fg(AnsiString &output) const;
    void report_bg(AnsiString &output) const;
};

class Ansi final
{
private:
    raw_ansi ansi_;

public:
    void process_code(int code);
    inline raw_ansi get_raw() const { return ansi_; }
    bool operator==(const Ansi &other) const { return ansi_ == other.ansi_; }
    bool operator!=(const Ansi &other) const { return ansi_ != other.ansi_; }
};

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
TextBuffer normalizeAnsi(const QStringRef &);
TextBuffer normalizeAnsi(const QString &str);

#define DEFINE_CHAR_CONST(NAME, val) \
    static constexpr const char C_##NAME{(val)}; \
    static constexpr const QChar QC_##NAME{(C_##NAME)}; \
    static constexpr const char ARR_##NAME[2]{C_##NAME, C_NUL}; \
    static constexpr const char *const S_##NAME{ARR_##NAME}; \
    static constexpr const std::string_view SV_##NAME { ARR_##NAME }

// TODO: put these in a string constants namespace
static constexpr const char C_NUL = 0;
DEFINE_CHAR_CONST(ESC, '\x1b');
DEFINE_CHAR_CONST(CARRIAGE_RETURN, '\r');
DEFINE_CHAR_CONST(NBSP, static_cast<char>('\xa0'));
DEFINE_CHAR_CONST(NEWLINE, '\n');
DEFINE_CHAR_CONST(OPEN_BRACKET, '[');
DEFINE_CHAR_CONST(SEMICOLON, ';');
DEFINE_CHAR_CONST(SPACE, ' ');
DEFINE_CHAR_CONST(TAB, '\t');
static_assert(C_SPACE == 0x20);

#undef DEFINE_CHAR_CONST

class AnsiStringToken final
{
public:
    using size_type = decltype(std::declval<QString>().size());

    enum class TokenTypeEnum { ANSI, CONTROL, NEWLINE, SPACE, WORD };
    TokenTypeEnum type = TokenTypeEnum::ANSI; // There is no good default value.

private:
    /* TODO: convert to QStringRef? */
    const QString *text_;
    const size_type offset_;
    const size_type length_;

public:
    explicit AnsiStringToken(TokenTypeEnum _type,
                             const QString &_text,
                             size_type _offset,
                             size_type _length);
    size_type length() const { return length_; }
    size_type start_offset() const { return offset_; }
    size_type end_offset() const { return offset_ + length_; }

public:
    QChar at(const size_type pos) const
    {
        assert(isClamped(pos, 0, length_));
        return text_->at(offset_ + pos);
    }
    QChar operator[](const size_type pos) const { return at(pos); }

public:
    auto begin() const { return text_->begin() + offset_; }
    auto end() const { return begin() + length_; }

public:
    QStringRef getQStringRef() const;
    bool isAnsiCsi() const;
};

struct AnsiTokenizer final
{
    class Iterator final
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
            next();
            return *this;
        }

        bool hasNext() const { return pos_ < str_.size(); }
        AnsiStringToken next();
        AnsiStringToken getCurrent();

    private:
        enum class ResultEnum { KEEPGOING, STOP };

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
        size_type skip_ansi();
        static bool isControl(const QChar &c) { return std::iscntrl(c.toLatin1()) && c != QC_NBSP; }
        size_type skip_control();
        size_type skip_space();
        size_type skip_word();
    };

    const QString &str_;
    AnsiTokenizer(QString &&) = delete;
    explicit AnsiTokenizer(const QString &_str)
        : str_{_str}
    {}

    Iterator begin() { return Iterator{str_, 0}; }
    auto end() { return nullptr; }
};

static constexpr int tab_advance(int col)
{
    return 8 - (col % 8);
}

static constexpr int next_tab_stop(int col)
{
    return col + tab_advance(col);
}
static_assert(next_tab_stop(0) == 8);
static_assert(next_tab_stop(1) == 8);
static_assert(next_tab_stop(7) == 8);
static_assert(next_tab_stop(8) == 16);
static_assert(next_tab_stop(9) == 16);
static_assert(next_tab_stop(15) == 16);
