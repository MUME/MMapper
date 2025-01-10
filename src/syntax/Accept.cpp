// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Accept.h"

#include <memory>
#include <ostream>

namespace syntax {

Accept::Accept(Accept::Function fn, std::string help)
    : m_function(std::make_shared<Function>(std::move(fn)))
    , m_help(std::move(help))
{
    if (*m_function == nullptr) {
        throw NullPointerException();
    }
}

void Accept::call(User &user, const Pair *const matched) const
{
    m_function->operator()(user, matched);
}

std::ostream &operator<<(std::ostream &os, const Accept &accept)
{
    return os << "Accept{" << accept.m_help << "}";
}

} // namespace syntax
