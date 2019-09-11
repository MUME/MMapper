#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "../global/TextUtils.h"

namespace syntax {
class ParserInput final
{
private:
    std::shared_ptr<const std::vector<std::string>> m_vector;
    size_t m_beg;
    size_t m_end;

public:
    explicit ParserInput(std::shared_ptr<const std::vector<std::string>> v);
    DEFAULT_RULE_OF_5(ParserInput);

    bool empty() const { return m_beg == m_end; }
    size_t size() const { return m_end - m_beg; }
    size_t length() const { return size(); }

    const std::string &front() const;
    const std::string &back() const;

    auto begin() const { return m_vector->data() + m_beg; }
    auto end() const { return m_vector->data() + m_end; }

    ParserInput subset(size_t a, size_t b) const;

    ParserInput left(size_t n) const;
    ParserInput mid(size_t n) const;
    ParserInput right(size_t n) const;
    ParserInput rmid(size_t n) const;

    void concatenate_into(std::ostream &os) const;

    std::string concatenate() const;

    ParserInput before(const ParserInput &other) const;
    bool isSubsetOf(const ParserInput &parent) const;

    friend std::ostream &operator<<(std::ostream &os, const ParserInput &parserInput);
};

} // namespace syntax
