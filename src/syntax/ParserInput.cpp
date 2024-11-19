// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ParserInput.h"

#include "../global/PrintUtils.h"

#include <memory>
#include <ostream>
#include <sstream>
#include <vector>

namespace syntax {

ParserInput::ParserInput(std::shared_ptr<const std::vector<std::string>> v)
    : m_vector(std::move(v))
    , m_beg{0}
    , m_end{m_vector->size()}
{}

const std::string &ParserInput::front() const
{
    if (empty())
        throw std::runtime_error("empty");
    return m_vector->at(m_beg);
}

const std::string &ParserInput::back() const
{
    if (empty())
        throw std::runtime_error("empty");
    return m_vector->at(m_end - 1);
}

ParserInput ParserInput::subset(const size_t a, const size_t b) const
{
    assert(a <= size());
    assert(b <= size());
    assert(a <= b);

    ParserInput copy = *this;
    copy.m_beg = m_beg + a;
    copy.m_end = m_beg + b;
    return copy;
}

ParserInput ParserInput::left(const size_t n) const
{
    assert(n <= size());
    return subset(0, n);
}

ParserInput ParserInput::mid(const size_t n) const
{
    assert(n <= size());
    return subset(n, size());
}

ParserInput ParserInput::right(const size_t n) const
{
    assert(n <= size());
    return subset(size() - n, size());
}

ParserInput ParserInput::rmid(const size_t n) const
{
    assert(n <= size());
    return subset(0, size() - n);
}

void ParserInput::concatenate_into(std::ostream &os) const
{
    bool first = true;
    for (auto &s : *this) {
        if (first)
            first = false;
        else
            os << " ";

        print_string_smartquote(os, s);
    }
}

std::string ParserInput::concatenate() const
{
    if (empty())
        return {};

    std::ostringstream ss;
    concatenate_into(ss);
    return ss.str();
}

ParserInput ParserInput::before(const ParserInput &other) const
{
    assert(other.isSubsetOf(*this));
    assert(this->m_vector == other.m_vector);
    assert(m_beg <= other.m_beg);
    assert(other.m_end <= m_end);
    return subset(0, other.m_beg - m_beg);
}

bool ParserInput::isSubsetOf(const ParserInput &parent) const
{
    return m_vector == parent.m_vector && parent.m_beg <= m_beg && m_end <= parent.m_end;
}

std::ostream &operator<<(std::ostream &os, const ParserInput &parserInput)
{
    os << "[";
    parserInput.concatenate_into(os);
    os << "]";
    return os;
}

} // namespace syntax
