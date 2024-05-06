#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/NullPointerException.h"
#include "ParserInput.h"
#include "Sublist.h"
#include "TokenMatcher.h"
#include "Value.h"

#include <functional>
#include <memory>
#include <stack>
#include <variant>

namespace syntax {

enum class NODISCARD MatchTypeEnum { Fail, Partial, Pass };

class NODISCARD TreeParser final
{
private:
    const SharedConstSublist m_syntaxRoot;
    User &m_user;

public:
    explicit TreeParser(SharedConstSublist syntaxRoot, User &user);

    NODISCARD bool parse(const ParserInput &input);

private:
    NODISCARD bool syntaxOnly(const ParserInput &input);

    NODISCARD ParseResult syntaxRecurseFirst(const Sublist &node,
                                             const ParserInput &input,
                                             const Pair *matchedArgs);
    NODISCARD ParseResult recurseNewSublist(const Sublist &node,
                                            const ParserInput &input,
                                            const Pair *matchedArgs);
    NODISCARD ParseResult recurseTokenMatcher(const Sublist &node,
                                              const ParserInput &input,
                                              const Pair *matchedArgs);
    NODISCARD ParseResult syntaxRecurseNext(const Sublist &node,
                                            const ParserInput &input,
                                            const Pair *matchedArgs);
    NODISCARD ParseResult recurseAccept(const Sublist &node,
                                        const ParserInput &input,
                                        const Pair *matchedArgs);

private:
    class HelpCommon;
    void help(const ParserInput &input, bool isFull);
};

} // namespace syntax
