#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <functional>
#include <memory>
#include <stack>
#include <variant>

#include "Accept.h"
#include "TokenMatcher.h"
#include "Value.h"

namespace syntax {

class Sublist;
using SharedConstSublist = std::shared_ptr<const Sublist>;
class TreeParser;

class Sublist final
{
private:
    friend class TreeParser;

    using Car = std::variant<TokenMatcher, SharedConstSublist>;
    using Cdr = std::variant<Accept, SharedConstSublist>;
    Car m_car;
    Cdr m_cdr;
    bool m_isComplete = false;

public:
    Sublist(TokenMatcher car, SharedConstSublist cdr);
    Sublist(TokenMatcher car, Accept cdr);
    Sublist(SharedConstSublist car, SharedConstSublist cdr);
    Sublist(SharedConstSublist car, Accept cdr);

private:
    // It has either a nested sublist, or a TokenMatcher.
    NODISCARD bool holdsNestedSublist() const
    {
        return std::holds_alternative<SharedConstSublist>(m_car);
    }
    // It has either a next node, or an Accept function.
    NODISCARD bool hasNextNode() const { return std::holds_alternative<SharedConstSublist>(m_cdr); }

    NODISCARD const SharedConstSublist &getNestedSublist() const
    {
        return std::get<SharedConstSublist>(m_car);
    }
    NODISCARD const SharedConstSublist &getNext() const
    {
        return std::get<SharedConstSublist>(m_cdr);
    }

    NODISCARD const TokenMatcher &getTokenMatcher() const { return std::get<TokenMatcher>(m_car); }
    NODISCARD const Accept &getAcceptFn() const { return std::get<Accept>(m_cdr); }

    // This is probably redundant.
    void checkCompleteness();

private:
    std::ostream &to_stream(std::ostream &os) const;

public:
    friend std::ostream &operator<<(std::ostream &os, const Sublist &sublist)
    {
        return sublist.to_stream(os);
    }
};

template<typename First>
NODISCARD SharedConstSublist buildSyntax(First &&first, Accept second)
{
    return std::make_shared<const Sublist>(std::forward<First>(first), std::move(second));
}

template<typename First>
NODISCARD SharedConstSublist buildSyntax(First &&first, SharedConstSublist second)
{
    assert(second != nullptr);
    return std::make_shared<const Sublist>(std::forward<First>(first),
                                           std::make_shared<const Sublist>(std::move(second),
                                                                           nullptr));
}

template<typename First, typename Second, typename... Rest>
NODISCARD SharedConstSublist buildSyntax(First &&first, Second &&second, Rest &&...rest)
{
    SharedConstSublist cdr = buildSyntax(std::forward<Second>(second), std::forward<Rest>(rest)...);
    return std::make_shared<const Sublist>(std::forward<First>(first), std::move(cdr));
}

NODISCARD std::string processSyntax(const syntax::SharedConstSublist &syntax,
                                    const std::string &name,
                                    const StringView &args);

} // namespace syntax
