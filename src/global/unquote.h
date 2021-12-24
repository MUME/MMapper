#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "macros.h"

struct NODISCARD UnquoteFailureReason final : public std::string
{
    explicit UnquoteFailureReason(std::string s)
        : basic_string(std::move(s))
    {}
};

using VectorOfStrings = std::vector<std::string>;
struct NODISCARD UnquoteResult final
{
private:
    std::variant<VectorOfStrings, UnquoteFailureReason> m_var;

public:
    explicit UnquoteResult(VectorOfStrings var)
        : m_var(std::move(var))
    {}
    explicit UnquoteResult(UnquoteFailureReason var)
        : m_var(std::move(var))
    {}

    NODISCARD bool has_value() const { return m_var.index() == 0; }
    explicit operator bool() const { return has_value(); }

    NODISCARD const VectorOfStrings &getVectorOfStrings() const { return std::get<0>(m_var); }
    NODISCARD const UnquoteFailureReason &getUnquoteFailureReason() const
    {
        return std::get<1>(m_var);
    }
};

NODISCARD extern UnquoteResult unquote(const std::string_view input,
                                       bool allowUnbalancedQuotes,
                                       bool allowEmbeddedNull);

namespace test {
void test_unquote() noexcept;
} // namespace test
