#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <functional>
#include <memory>
#include <stack>
#include <variant>

#include "../global/NullPointerException.h"
#include "ParserInput.h"
#include "Sublist.h"
#include "TokenMatcher.h"
#include "Value.h"

namespace syntax {

enum class MatchTypeEnum { Fail, Partial, Pass };

class TreeParser final
{
private:
    const SharedConstSublist m_syntaxRoot;
    User &m_user;

public:
    explicit TreeParser(SharedConstSublist syntaxRoot, User &user);

    bool parse(const ParserInput &input);

private:
    bool syntaxOnly(const ParserInput &input);

    ParseResult syntaxRecurseFirst(const Sublist &node,
                                   const ParserInput &input,
                                   const Pair *matchedArgs);
    ParseResult recurseNewSublist(const Sublist &node,
                                  const ParserInput &input,
                                  const Pair *matchedArgs);
    ParseResult recurseTokenMatcher(const Sublist &node,
                                    const ParserInput &input,
                                    const Pair *matchedArgs);
    ParseResult syntaxRecurseNext(const Sublist &node,
                                  const ParserInput &input,
                                  const Pair *matchedArgs);
    ParseResult recurseAccept(const Sublist &node,
                              const ParserInput &input,
                              const Pair *matchedArgs);

private:
    class HelpCommon;
    void help(const ParserInput &input, bool isFull);
};

} // namespace syntax
