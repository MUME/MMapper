// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "PrintUtils.h"

#include "Charset.h"

#include <ostream>

// std::isprint is only ASCII

bool requiresQuote(const std::string_view str)
{
    for (const char c : str) {
        if (isSpace(c) || !isPrintLatin1(c)) {
            return true;
        }
    }
    return false;
}

namespace token_stream {
ICharTokenStream::~ICharTokenStream() = default;
PassThruCharTokenStream::~PassThruCharTokenStream() = default;
CallbackCharTokenStream::~CallbackCharTokenStream() = default;

/* lvalue version */
void print_char(ICharTokenStream &os, const char c, const bool doubleQuote)
{
    using namespace char_consts;
    switch (c) {
    case C_ESC:
        os.getEsc() << "\\e"; // Not valid C++, but borrowed from /bin/echo.
        break;
    case C_ALERT:
        os.getEsc() << "\\a";
        break;
    case C_BACKSPACE:
        os.getEsc() << "\\b";
        break;
    case C_FORM_FEED:
        os.getEsc() << "\\f";
        break;
    case C_NEWLINE:
        os.getEsc() << "\\n";
        break;
    case C_CARRIAGE_RETURN:
        os.getEsc() << "\\r";
        break;
    case C_TAB:
        os.getEsc() << "\\t";
        break;
    case C_VERTICAL_TAB:
        os.getEsc() << "\\v";
        break;
    case C_BACKSLASH:
        os.getEsc() << "\\\\";
        break;
    case C_NUL:
        // NOTE: If we were generating C++, this could emit incorrect results (e.g. '0' followed by '0'),
        // but this format only allows octal values with '\o###'.
        os.getEsc() << "\\0";
        break;
    default:
        if (isPrintLatin1(c)) {
            if (c == (doubleQuote ? C_DQUOTE : C_SQUOTE)) {
                auto esc = os.getEsc();
                esc << C_BACKSLASH;
                esc << c;
            } else {
                os.getNormal() << c;
            }
        } else {
            // NOTE: This form can generate invalid C++.
            char buf[8];
            std::sprintf(buf, "\\x%02X", (c & 0xFF)); // max 5 bytes including C_NUL
            os.getEsc() << buf;
        }
        break;
    }
}
/* lvalue version */
void print_string_quoted(ICharTokenStream &os, const std::string_view sv, const bool includeQuotes)
{
    if (includeQuotes) {
        os.getEsc() << char_consts::C_DQUOTE;
    }
    for (const auto &c : sv) {
        print_char(os, c, true);
    }
    if (includeQuotes) {
        os.getEsc() << char_consts::C_DQUOTE;
    }
}

} // namespace token_stream

std::ostream &print_char(std::ostream &os, const char c, const bool doubleQuote)
{
    using namespace token_stream;
    print_char(PassThruCharTokenStream{os}, c, doubleQuote);
    return os;
}

std::ostream &print_char_quoted(std::ostream &os, const char c)
{
    os << char_consts::C_SQUOTE;
    print_char(os, c, false);
    return os << char_consts::C_SQUOTE;
}

std::ostream &print_string_quoted(std::ostream &os, const std::string_view sv)
{
    using namespace token_stream;
    print_string_quoted(PassThruCharTokenStream{os}, sv);
    return os;
}

std::ostream &print_string_smartquote(std::ostream &os, const std::string_view sv)
{
    if (!requiresQuote(sv)) {
        return os << sv;
    }
    return print_string_quoted(os, sv);
}

void to_stream_as_reset(std::ostream &os, const AnsiSupportFlags supportFlags, const RawAnsi &ansi)
{
    const AnsiString str = ansi_transition(supportFlags, RawAnsi{}, ansi);
    const AnsiString copy = str.copy_as_reset();
    os << copy.c_str();
}

std::ostream &reset_ansi(std::ostream &os)
{
    const AnsiString reset = AnsiString::get_reset_string();
    return os << reset.c_str();
}
