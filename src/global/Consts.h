#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include <string_view>

#include <QString>

#define XFOREACH_CHAR_CONST(X) \
    X(ALERT, '\a') \
    X(AMPERSAND, '&') \
    X(ASTERISK, '*') \
    X(AT_SIGN, '@') \
    X(BACKSLASH, '\\') \
    X(BACKSPACE, '\b') \
    X(BACK_TICK, '`') \
    X(CARET, '^') \
    X(CARRIAGE_RETURN, '\r') \
    X(CLOSE_BRACKET, ']') \
    X(CLOSE_CURLY, '}') \
    X(CLOSE_PARENS, ')') \
    X(COLON, ':') \
    X(COMMA, ',') \
    X(DELETE, '\x7F') \
    X(DOLLAR_SIGN, '$') \
    X(DQUOTE, '\"') \
    X(EQUALS, '=') \
    X(ESC, '\x1B') \
    X(EXCLAMATION, '!') \
    X(FORM_FEED, '\f') \
    X(GREATER_THAN, '>') \
    X(LESS_THAN, '<') \
    X(MINUS_SIGN, '-') \
    X(NBSP, static_cast<char>('\xA0')) \
    X(NEWLINE, '\n') \
    X(OPEN_BRACKET, '[') \
    X(OPEN_CURLY, '{') \
    X(OPEN_PARENS, '(') \
    X(PERCENT_SIGN, '%') \
    X(PERIOD, '.') \
    X(PLUS_SIGN, '+') \
    X(POUND_SIGN, '#') \
    X(QUESTION_MARK, '?') \
    X(SEMICOLON, ';') \
    X(SLASH, '/') \
    X(SPACE, ' ') \
    X(SQUOTE, '\'') \
    X(TAB, '\t') \
    X(TILDE, '~') \
    X(UNDERSCORE, '_') \
    X(VERTICAL_BAR, '|') \
    X(VERTICAL_TAB, '\v')

namespace char_consts {
static inline constexpr const char C_NUL = 0;
static inline constexpr const auto C_CTRL_U = '\x15';
static inline constexpr const auto C_CTRL_W = '\x17';

#define X_DEFINE_CHAR_CONST(NAME, val) static inline constexpr const char C_##NAME{(val)};
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
} // namespace char_consts

namespace string_consts {
#define X_DEFINE_CHAR_CONST(NAME, val) \
    static inline constexpr const char ARR_##NAME[2]{char_consts::C_##NAME, char_consts::C_NUL};
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
#define X_DEFINE_CHAR_CONST(NAME, val) \
    static inline constexpr const char *const S_##NAME{ARR_##NAME};
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
#define X_DEFINE_CHAR_CONST(NAME, val) \
    static inline constexpr const std::string_view SV_##NAME{ARR_##NAME, 1};
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
} // namespace string_consts

namespace mmqt {
#define X_DEFINE_CHAR_CONST(NAME, val) \
    static inline constexpr const QChar QC_##NAME{(char_consts::C_##NAME)};
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
#define X_DEFINE_CHAR_CONST(NAME, val) \
    static inline const QString QS_##NAME = QString::fromLatin1(string_consts::S_##NAME);
XFOREACH_CHAR_CONST(X_DEFINE_CHAR_CONST);
#undef X_DEFINE_CHAR_CONST
} // namespace mmqt

namespace string_consts {
static inline constexpr const char ARR_TWO_SPACES[3]{char_consts::C_SPACE,
                                                     char_consts::C_SPACE,
                                                     char_consts::C_NUL};
static inline constexpr const char *const S_TWO_SPACES{ARR_TWO_SPACES};
static inline constexpr std::string_view SV_TWO_SPACES{ARR_TWO_SPACES, 2};

static inline constexpr const char ARR_CRLF[3]{char_consts::C_CARRIAGE_RETURN,
                                               char_consts::C_NEWLINE,
                                               char_consts::C_NUL};
static inline constexpr const char *const S_CRLF{ARR_CRLF};
static inline constexpr std::string_view SV_CRLF{ARR_CRLF, 2};

} // namespace string_consts
