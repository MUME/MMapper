#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Color.h"
#include "RuleOf5.h"
#include "utils.h"

#include <string_view>
#include <vector>

// TODO: rename this, but to what? NamedColorHandle?
class NODISCARD XNamedColor final
{
public:
    static constexpr size_t UNINITIALIZED = 0;

private:
    const size_t m_index;

public:
    XNamedColor() = delete;
    XNamedColor(std::nullptr_t) = delete;
    explicit XNamedColor(std::string_view name);
    template<size_t N>
    explicit XNamedColor(const char name[N])
        : XNamedColor(std::string_view(name, N))
    {}

public:
    ~XNamedColor();

public:
    DEFAULT_CTORS(XNamedColor);
    DELETE_ASSIGN_OPS(XNamedColor);

public:
    NODISCARD bool isInitialized() const noexcept { return getIndex() != UNINITIALIZED; }
    NODISCARD size_t getIndex() const noexcept { return m_index; }

public:
    NODISCARD std::string getName() const;
    NODISCARD Color getColor() const;
    void setColor(Color c);

public:
    NODISCARD explicit operator Color() const { return getColor(); }
    XNamedColor &operator=(Color c)
    {
        setColor(c);
        return *this;
    }

public:
    NODISCARD bool operator==(const XNamedColor &rhs) const { return m_index == rhs.m_index; }
    NODISCARD bool operator!=(const XNamedColor &rhs) const { return !(rhs == *this); }

public:
    // NOTE: This allocates memory.
    NODISCARD static std::vector<std::string> getAllNames();
};
