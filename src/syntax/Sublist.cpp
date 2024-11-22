// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Sublist.h"

#include "../global/PrintUtils.h"

namespace syntax {
Sublist::Sublist(TokenMatcher car, SharedConstSublist cdr)
    : m_car(std::move(car))
    , m_cdr(std::move(cdr))
{
    checkCompleteness();
}

Sublist::Sublist(TokenMatcher car, Accept cdr)
    : m_car(std::move(car))
    , m_cdr(std::move(cdr))
    , m_isComplete{true}
{
    if ((false))
        checkCompleteness(); // already complete
}

Sublist::Sublist(SharedConstSublist car, SharedConstSublist cdr)
    : m_car(std::move(car))
    , m_cdr(std::move(cdr))
{
    if (std::get<SharedConstSublist>(m_car) == nullptr)
        throw NullPointerException();
    checkCompleteness();
}

Sublist::Sublist(SharedConstSublist car, Accept cdr)
    : m_car(std::move(car))
    , m_cdr(std::move(cdr))
{
    if (std::get<SharedConstSublist>(m_car) == nullptr)
        throw NullPointerException();
    checkCompleteness();
}

void Sublist::checkCompleteness()
{
    if (m_isComplete)
        return;

    bool hasToken = false;
    bool hasAccept = false;
    if (holdsNestedSublist()) {
        m_isComplete |= getNestedSublist()->m_isComplete;
    } else {
        hasToken = true;
    }

    if (hasNextNode()) {
        if (const auto &tmp = getNext())
            m_isComplete |= tmp->m_isComplete;
    } else {
        hasAccept = true;
    }

    m_isComplete |= hasToken && hasAccept;
    if (!m_isComplete)
        throw std::runtime_error("syntax is not complete");
}

std::ostream &Sublist::to_stream(std::ostream &os) const
{
    if (holdsNestedSublist()) {
        os << "\n(";
        getNestedSublist()->to_stream(os);
        os << ")\n";
    } else {
        os << getTokenMatcher();
    }

    if (hasNextNode()) {
        if (const auto &ptr = getNext()) {
            os << " ";
            ptr->to_stream(os);
        }
    } else {
        os << " . fn(" << QuotedString(getAcceptFn().getHelp()) << ")";
    }
    return os;
}

} // namespace syntax
