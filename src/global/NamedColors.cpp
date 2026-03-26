// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "NamedColors.h"

#include "../opengl/UboBlocks.h"
#include "EnumIndexedArray.h"

#include <cassert>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct NODISCARD GlobalData final
{
    using ColorVector = std::vector<Color>;
    using InitArray = EnumIndexedArray<bool, NamedColorEnum, NUM_NAMED_COLORS>;
    using Map = std::map<std::string, NamedColorEnum>;
    using NamesVector = std::vector<std::string>;

private:
    Map m_map;
    NamesVector m_names;
    ColorVector m_colors;
    Legacy::NamedColorsBlock m_colorsBlock;
    InitArray m_initialized;

private:
    NODISCARD static size_t getIndex(NamedColorEnum e) { return static_cast<uint8_t>(e); }

public:
    GlobalData()
    {
        m_colors.resize(NUM_NAMED_COLORS);
        m_names.resize(NUM_NAMED_COLORS);

        static const auto white = Colors::white;
        static const auto transparent_black = Colors::black.withAlpha(0);
        assert(white.getRGBA() == ~0u);
        assert(transparent_black.getRGBA() == 0);

        const auto init = [this](const NamedColorEnum id, const std::string_view name) {
            const auto idx = getIndex(id);
            const auto color = (id == NamedColorEnum::TRANSPARENT) ? transparent_black : white;

            m_colors[idx] = color;
            m_colorsBlock.colors[idx] = color.getVec4();
            m_names[idx] = name;
            m_map.emplace(name, id);
        };

#define X_INIT_MAP(_id, _name) init((NamedColorEnum::_id), std::string_view{_name});
        XFOREACH_NAMED_COLOR_OPTIONS(X_INIT_MAP)
#undef X_INIT_MAP

        init(NamedColorEnum::DEFAULT, ".default");
        m_initialized[NamedColorEnum::DEFAULT] = true;
        m_initialized[NamedColorEnum::TRANSPARENT] = true;

        // Sane defaults for weather colors
        std::ignore = setColor(NamedColorEnum::WEATHER_DAWN,
                               Color(102, 76, 51, 25)); // 0.4, 0.3, 0.2, 0.1
        std::ignore = setColor(NamedColorEnum::WEATHER_DUSK,
                               Color(76, 51, 102, 51)); // 0.3, 0.2, 0.4, 0.2
        std::ignore = setColor(NamedColorEnum::WEATHER_NIGHT,
                               Color(13, 13, 51, 89)); // 0.05, 0.05, 0.2, 0.35
        std::ignore = setColor(NamedColorEnum::WEATHER_NIGHT_MOON,
                               Color(13, 13, 64, 77)); // 0.05, 0.05, 0.25, 0.3
    }

public:
    NODISCARD bool isInitialized(const NamedColorEnum id) const { return m_initialized[id]; }

public:
    NODISCARD Color getColor(const NamedColorEnum id) { return m_colors.at(getIndex(id)); }
    NODISCARD bool setColor(const NamedColorEnum id, Color c)
    {
        if (id == NamedColorEnum::DEFAULT || id == NamedColorEnum::TRANSPARENT) {
            return false;
        }

        const auto idx = getIndex(id);
        m_colors.at(idx) = c;
        m_colorsBlock.colors.at(idx) = c.getVec4();
        m_initialized.at(id) = true;
        return true;
    }

public:
    NODISCARD const std::string &getName(const NamedColorEnum id) const
    {
        return m_names[getIndex(id)];
    }

    NODISCARD const std::vector<std::string> &getAllNames() const { return m_names; }
    NODISCARD const std::vector<Color> &getAllColors() const { return m_colors; }
    NODISCARD const Legacy::NamedColorsBlock &getAllColorsAsBlock() const { return m_colorsBlock; }

public:
    NODISCARD std::optional<NamedColorEnum> lookup(std::string_view name) const
    {
        const auto &map = m_map;
        if (const auto it = map.find(std::string(name)); it != map.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

NODISCARD static GlobalData &getGlobalData()
{
    static GlobalData globalData;
    return globalData;
}

XNamedColor::~XNamedColor() = default;

std::optional<XNamedColor> XNamedColor::lookup(std::string_view sv)
{
    if (auto opt = getGlobalData().lookup(sv)) {
        return XNamedColor{*opt};
    }
    return std::nullopt;
}

bool XNamedColor::isInitialized() const
{
    return getGlobalData().isInitialized(m_value);
}

const std::string &XNamedColor::getName() const
{
    return getGlobalData().getName(m_value);
}

Color XNamedColor::getColor() const
{
    assert(isInitialized());
    return getGlobalData().getColor(m_value);
}

bool XNamedColor::setColor(const Color new_color)
{
    return getGlobalData().setColor(m_value, new_color);
}

const std::vector<std::string> &XNamedColor::getAllNames()
{
    return getGlobalData().getAllNames();
}

const std::vector<Color> &XNamedColor::getAllColors()
{
    return getGlobalData().getAllColors();
}

const Legacy::NamedColorsBlock &XNamedColor::getAllColorsAsBlock()
{
    return getGlobalData().getAllColorsAsBlock();
}
