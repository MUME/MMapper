#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <functional>
#include <memory>
#include <ostream>

#include "../global/NullPointerException.h"
#include "User.h"
#include "Value.h"

namespace syntax {

class NODISCARD Accept final
{
public:
    using Function = std::function<void(User &user, const Pair *)>;

private:
    std::shared_ptr<const Function> m_function;
    std::string m_help;

public:
    Accept() = delete;
    DEFAULT_RULE_OF_5(Accept);

    explicit Accept(Function fn, std::string help);
    void operator()(User &user, const Pair *const matched) const { call(user, matched); }
    void call(User &user, const Pair *matched) const;
    NODISCARD const std::string &getHelp() const { return m_help; }

public:
    friend std::ostream &operator<<(std::ostream &os, const Accept &accept);
};

} // namespace syntax
