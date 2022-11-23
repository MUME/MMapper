#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <any>
#include <cassert>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../global/RuleOf5.h"
#include "../global/StringView.h"
#include "../global/TextUtils.h"
#include "Accept.h"
#include "IArgument.h"
#include "IMatchErrorLogger.h"
#include "MatchResult.h"
#include "ParseResult.h"
#include "ParserInput.h"
#include "User.h"
#include "Value.h"

namespace syntax {

// This was originally a type-erased value semantics container based on Sean Parent's
// Runtime Polymorphism talk, but it has been converted to a wrapper of IArgument
// to make it easier to find and modify arguments. Some parts of the original
// design still remain.
class NODISCARD TokenMatcher final
{
private:
    std::shared_ptr<const IArgument> m_self;

public:
    TokenMatcher() = delete;
    DEFAULT_RULE_OF_5(TokenMatcher);
    explicit TokenMatcher(std::shared_ptr<const IArgument> ptr)
        : m_self(std::move(ptr))
    {}

public:
    template<typename T>
    NODISCARD static TokenMatcher alloc(T &&x) = delete;

    template<typename T, typename... Args>
    NODISCARD static TokenMatcher alloc(Args &&...args)
    {
        auto ptr = std::make_shared<const T>(std::forward<Args>(args)...);
        return TokenMatcher{std::move(ptr)};
    }

    template<typename T>
    NODISCARD static TokenMatcher alloc_copy(T &&val)
    {
        auto ptr = std::make_shared<const T>(std::forward<T>(val));
        return TokenMatcher{std::move(ptr)};
    }

public:
    NODISCARD MatchResult tryMatch(const ParserInput &sv, IMatchErrorLogger *logger) const;

public:
    friend std::ostream &operator<<(std::ostream &os, const TokenMatcher &token);
};

} // namespace syntax
