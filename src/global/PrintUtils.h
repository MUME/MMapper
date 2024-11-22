#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "AnsiTextUtils.h"
#include "utils.h"

#include <ostream>

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

namespace token_stream {

enum class NODISCARD CharTokenTypeEnum : uint8_t { Normal, Escaped };
struct NODISCARD ICharTokenStream
{
private:
    virtual void virt_append(CharTokenTypeEnum, char) = 0;
    virtual void virt_append(CharTokenTypeEnum, std::string_view) = 0;

public:
    virtual ~ICharTokenStream();

public:
    void appendNormal(char c) { virt_append(CharTokenTypeEnum::Normal, c); }
    void appendNormal(std::string_view sv) { virt_append(CharTokenTypeEnum::Normal, sv); }
    void appendEscaped(char c) { virt_append(CharTokenTypeEnum::Escaped, c); }
    void appendEscaped(std::string_view sv) { virt_append(CharTokenTypeEnum::Escaped, sv); }

    class NODISCARD Helper final
    {
    private:
        friend ICharTokenStream;
        ICharTokenStream &m_stream;
        CharTokenTypeEnum m_type;

    private:
        explicit Helper(ICharTokenStream &stream, CharTokenTypeEnum type)
            : m_stream{stream}
            , m_type{type}
        {}

    public:
        Helper &operator<<(char c)
        {
            m_stream.virt_append(m_type, c);
            return *this;
        }
        Helper &operator<<(std::string_view sv)
        {
            m_stream.virt_append(m_type, sv);
            return *this;
        }
    };
    NODISCARD Helper getNormal() { return Helper{*this, CharTokenTypeEnum::Normal}; }
    NODISCARD Helper getEsc() { return Helper{*this, CharTokenTypeEnum::Escaped}; }
};

struct NODISCARD PassThruCharTokenStream final : public ICharTokenStream
{
private:
    std::ostream &m_os;

public:
    NODISCARD explicit PassThruCharTokenStream(std::ostream &os)
        : m_os{os}
    {}
    ~PassThruCharTokenStream() final;

private:
    void virt_append(CharTokenTypeEnum, char c) final { m_os << c; }
    void virt_append(CharTokenTypeEnum, std::string_view sv) final { m_os << sv; }
};

struct NODISCARD CallbackCharTokenStream final : public ICharTokenStream
{
private:
    using CharFn = std::function<void(CharTokenTypeEnum, char)>;
    using StringFn = std::function<void(CharTokenTypeEnum, std::string_view)>;
    CharFn m_charFn;
    StringFn m_stringFn;

public:
    NODISCARD explicit CallbackCharTokenStream(CharFn cfn, StringFn sfn)
        : m_charFn{cfn}
        , m_stringFn{sfn}
    {}
    ~CallbackCharTokenStream() final;

private:
    void virt_append(CharTokenTypeEnum type, char c) final { m_charFn(type, c); }
    void virt_append(CharTokenTypeEnum type, std::string_view sv) final { m_stringFn(type, sv); }
};

/* lvalue version */
void print_char(ICharTokenStream &os, char c, bool doubleQuote);
/* rvalue version */
inline void print_char(ICharTokenStream &&os, char c, bool doubleQuote)
{
    /* lvalue version */
    print_char(os, c, doubleQuote);
}

/* lvalue version */
void print_string_quoted(ICharTokenStream &os, std::string_view sv, bool includeQuotes);
/* rvalue version */
inline void print_string_quoted(ICharTokenStream &&os, std::string_view sv, bool includeQuotes)
{
    /* lvalue version */
    print_string_quoted(os, sv, includeQuotes);
}

/* lvalue version */
inline void print_string_quoted(ICharTokenStream &os, const std::string_view sv)
{
    print_string_quoted(os, sv, true);
}
inline void print_string_quoted(ICharTokenStream &&os, const std::string_view sv)
{
    /* lvalue version */
    print_string_quoted(os, sv, true);
}

} // namespace token_stream

void to_stream_as_reset(std::ostream &os, AnsiSupportFlags supportFlags, const RawAnsi &ansi);

std::ostream &reset_ansi(std::ostream &os);
