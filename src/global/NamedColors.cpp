// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "NamedColors.h"

#include <cassert>
#include <map>
#include <optional>
#include <string>
#include <vector>

using Index = size_t;
using Map = std::map<std::string, Index>;

using ColorVector = std::vector<Color>;
using ReverseLookup = std::vector<std::optional<Map::iterator>>;

struct GlobalData final
{
private:
    ColorVector colors;
    Map map;
    ReverseLookup reverseLookup;

public:
    Color &getColor(const Index index)
    {
        if (index == XNamedColor::UNINITIALIZED) {
            throw std::invalid_argument("index");
        }
        return colors.at(index - 1);
    }
    std::string getName(const Index index) const
    {
        if (index == XNamedColor::UNINITIALIZED) {
            throw std::invalid_argument("index");
        }
        if (auto opt = reverseLookup.at(index - 1))
            return opt.value()->first;
        return {};
    }

private:
    Index verifyIndex(Index index) const;
    Index allocNewIndex(std::string_view name);

public:
    Index lookupOrCreateIndex(std::string_view name);

public:
    std::vector<std::string> getAllNames() const
    {
        std::vector<std::string> result;
        result.reserve(map.size());
        for (const auto &kv : map) {
            result.emplace_back(kv.first);
        }
        return result;
    }
};

Index GlobalData::verifyIndex(const Index index) const
{
    if (index == XNamedColor::UNINITIALIZED) {
        throw std::invalid_argument("index");
    }

    const auto &rev = reverseLookup.at(index - 1);
    assert(rev.has_value() && rev.value()->second == index);
    return index;
}

Index GlobalData::allocNewIndex(std::string_view name)
{
    assert(colors.size() == reverseLookup.size());
    const Index index = colors.size() + 1;
    colors.emplace_back();

    const auto emplace_result = map.emplace(name, index);
    if (!emplace_result.second) {
        assert(false);
        reverseLookup.emplace_back();
        return XNamedColor::UNINITIALIZED;
    }

    reverseLookup.emplace_back(emplace_result.first);
    return verifyIndex(index);
}

Index GlobalData::lookupOrCreateIndex(std::string_view name)
{
    const auto it = map.find(std::string(name));
    if (it != map.end()) {
        return verifyIndex(it->second);
    }

    return allocNewIndex(name);
}

static GlobalData &getGlobalData()
{
    static GlobalData globalData;
    return globalData;
}

XNamedColor::XNamedColor(std::string_view name)
    : m_index{getGlobalData().lookupOrCreateIndex(name)}
{}

XNamedColor::~XNamedColor() = default;

std::string XNamedColor::getName() const
{
    if (!isInitialized()) {
        assert(false);
        return {};
    }

    return getGlobalData().getName(getIndex());
}

Color XNamedColor::getColor() const
{
    if (!isInitialized()) {
        assert(false);
        return {};
    }

    return getGlobalData().getColor(getIndex());
}

void XNamedColor::setColor(const Color new_color)
{
    if (!isInitialized()) {
        assert(false);
        return;
    }

    getGlobalData().getColor(getIndex()) = new_color;
}

std::vector<std::string> XNamedColor::getAllNames()
{
    return getGlobalData().getAllNames();
}
