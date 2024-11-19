#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/TextUtils.h"

#include <cassert>
#include <memory>
#include <ostream>
#include <vector>

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

    NODISCARD bool empty() const { return m_beg == m_end; }
    NODISCARD size_t size() const { return m_end - m_beg; }
    NODISCARD size_t length() const { return size(); }

    NODISCARD const std::string &front() const;
    NODISCARD const std::string &back() const;

    NODISCARD auto begin() const { return m_vector->data() + m_beg; }
    NODISCARD auto end() const { return m_vector->data() + m_end; }

    NODISCARD ParserInput subset(size_t a, size_t b) const;

    NODISCARD ParserInput left(size_t n) const;
    NODISCARD ParserInput mid(size_t n) const;
    NODISCARD ParserInput right(size_t n) const;
    NODISCARD ParserInput rmid(size_t n) const;

    void concatenate_into(std::ostream &os) const;

    NODISCARD std::string concatenate() const;

    NODISCARD ParserInput before(const ParserInput &other) const;
    NODISCARD bool isSubsetOf(const ParserInput &parent) const;

    friend std::ostream &operator<<(std::ostream &os, const ParserInput &parserInput);
};

} // namespace syntax
