#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "AnsiTextUtils.h"
#include "Badge.h"
#include "ConfigEnums.h"
#include "TaggedString.h"
#include "macros.h"

#include <cinttypes>
#include <iosfwd>
#include <optional>
#include <ostream>
#include <sstream>
#include <variant>

class QuotedString;
class SmartQuotedString;

template<typename T>
struct NODISCARD ColoredValue final
{
    RawAnsi color;
    T value{};
    explicit ColoredValue(const RawAnsi c, const T v)
        : color{c}
        , value{v}
    {}
};

template<typename T>
ColoredValue(RawAnsi, T) -> ColoredValue<T>;

struct NODISCARD ColoredQuotedStringView final
{
    RawAnsi normal;
    RawAnsi escapes;
    std::string_view value;

    explicit ColoredQuotedStringView(const RawAnsi normal_,
                                     const RawAnsi escapes_,
                                     const std::string_view sv)
        : normal{normal_}
        , escapes{escapes_}
        , value{sv}
    {}
};

// AnsiOstream writes UTF8 bytes to the std::ostream.
//
// By default, all char-based string types (e.g. const char*, std::string, std::string_view, etc)
// are treated as Latin-1 and re-encoded as UTF-8. Users can avoid encoding surprises by specifying
// the encoding strings passed to the stream.
//
// Eventually this class will probably also support c++20 char8_t-based strings a UTF-8, which
// would allow writing: `os << "latin1-text: [\xFF]\n" << u8"utf8-text: [\xC3\xBF]\n";`
//
// Note: The code uses a private RAII helper to save/restore ANSI color. This is hidden as a
// private detail to help keep users from shooting themselves in the foot with the wrong lifetime.
//
// An ugly design choice: the various writeQuotedWithColor() overloads could be rewritten as a
// templated friend IO manipulator with overloaded operator<< that uses the RAII helper, but then
// the template would need to take care to avoid copying strings -and- avoid creating views of
// xvalue strings. Getting that right will take more coding effort than it's probably worth.
class NODISCARD AnsiOstream final
{
public:
    class NODISCARD NextStateRestorer final
    {
    private:
        AnsiOstream &m_aos;
        RawAnsi m_nextAnsi;

    public:
        explicit NextStateRestorer(Badge<AnsiOstream>, AnsiOstream &aos)
            : m_aos{aos}
            , m_nextAnsi{aos.getNextAnsi()}
        {}

        ~NextStateRestorer()
        {
            //
            m_aos.m_nextAnsi = m_nextAnsi;
        }
        DELETE_CTORS_AND_ASSIGN_OPS(NextStateRestorer);
    };

private:
    std::ostream &m_os;
    const AnsiSupportFlags m_supportFlags;
    RawAnsi m_currentAnsi;
    std::optional<RawAnsi> m_nextAnsi;
    bool m_hasNewline = false;

public:
    explicit AnsiOstream(std::ostream &os, const AnsiSupportFlags supportFlags)
        : m_os{os}
        , m_supportFlags{supportFlags}
    {}

    // TODO: Make the default based on user preferences? (We may need toUser and toMud prefs.)
    explicit AnsiOstream(std::ostream &os)
        : AnsiOstream{os, ANSI_COLOR_SUPPORT_HI}
    {}

    ~AnsiOstream() { close(); }
    DELETE_CTORS_AND_ASSIGN_OPS(AnsiOstream);

public:
    NODISCARD NextStateRestorer getStateRestorer()
    {
        return NextStateRestorer{Badge<AnsiOstream>{}, *this};
    }

private:
    NODISCARD std::ostream &getOstream() { return m_os; }
    NODISCARD AnsiSupportFlags getSupport() const { return m_supportFlags; }
    void transition(RawAnsi);
    void writeLowLevel(std::string_view);
    void close();

public:
    NODISCARD bool hasNewline() const { return m_hasNewline; }
    void writeNewline();

public:
    NODISCARD RawAnsi getNextAnsi() const { return m_nextAnsi.value_or(m_currentAnsi); }
    void setNextAnsi(const RawAnsi &ansi) { m_nextAnsi = ansi; }

public:
    void write(const RawAnsi &ansi) { setNextAnsi(ansi); }

    void write(char c);
    void write(char16_t codepoint);
    void write(char32_t codepoint);
    void write(std::string_view sv);

    template<typename T>
    auto write(const T n)
        -> std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<char32_t, std::decay_t<T>>
                             && sizeof(char) < sizeof(T))
                                || std::is_floating_point_v<T>,
                            void>
    {
        auto s = std::to_string(n);
        write(std::string_view{s});
    }

    template<size_t N>
    void write(const char (&buf)[N])
    {
        write(std::string_view{buf});
    }

public:
    void write(const QuotedString &);
    void write(const SmartQuotedString &);
    void write(const ColoredQuotedStringView s)
    {
        writeQuotedWithColor(s.normal, s.escapes, s.value);
    }

    template<typename T>
    void write(const ColoredValue<T> x)
    {
        writeWithColor(x.color, x.value);
    }

public:
    template<typename T>
    AnsiOstream &operator<<(const T &x)
    {
        write(x);
        return *this;
    }

public:
    template<typename T>
    void writeWithColor(const RawAnsi &ansi, const T &x)
    {
        auto raii = getStateRestorer();
        setNextAnsi(ansi);
        write(x);
    }
    template<typename T>
    void writeWithColor(const RawAnsi &ansi, const TaggedBoxedStringUtf8<T> &x)
    {
        auto raii = getStateRestorer();
        setNextAnsi(ansi);
        write(x.getStdStringViewUtf8());
    }

public:
    void writeQuotedWithColor(const RawAnsi &normalAnsi,
                              const RawAnsi &escapeAnsi,
                              std::string_view sv,
                              bool includeQuotes);
    void writeQuotedWithColor(const RawAnsi &normalAnsi,
                              const RawAnsi &escapeAnsi,
                              const std::string_view sv)
    {
        writeQuotedWithColor(normalAnsi, escapeAnsi, sv, true);
    }

public:
    void writeQuoted(std::string_view sv);
    void writeSmartQuoted(std::string_view sv);

public:
    // Parses and interprets ansi codes embedded in the input string, and combines them with
    // stream's current state, as if by writing alternating RawAnsi and string_view writes.
    //
    // Just as with regular RawAnsi writes, this function will ignores the REMOVAL of Ansi flags
    // that are NOT currently in use, and it ignores ADDITION of states that ARE currently in use.
    //
    // For example, if the current state is bold+italic+underline, then writing
    // "ESC[21;24mx" may cause the stream to behave "as if" you had written "ESC[0;3mx".
    //
    // Important: This purposely does not return to the previous stream state afterwards,
    // so the final ansi state can depend on the contents of the string. (If you want push/pop
    // behavior, consider saving the current state prior to calling this function and then
    // restore that state afterwards.)
    //
    // CAUTION: Ansi-codes must be complete; calling this function twice with string
    // that's split in the middle of an ANSI code will give the wrong result.
    void writeWithEmbeddedAnsi(std::string_view ansiColorString);

public:
    struct NODISCARD Endl final
    {};
    static inline const Endl endl;
    void write(Endl) { writeNewline(); }
};

inline void print_string_quoted(AnsiOstream &aos, std::string_view sv)
{
    aos.writeQuoted(sv);
}

namespace test {
extern void testAnsiOstream();
} // namespace test
