/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "ParseTree.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/property.h"
#include "../global/utils.h"
#include "roomcollection.h"

struct ParseTree::Pimpl
{
    explicit Pimpl() = default;
    virtual ~Pimpl() = default;
    virtual SharedRoomCollection insertRoom(ParseEvent &event) = 0;
    virtual void getRooms(AbstractRoomVisitor &stream, ParseEvent &event) = 0;
};

enum class MaskFlags : uint32_t {
    NONE = 0u,
    NAME = 0b001u,
    DESC = 0b010u,
    NAME_DESC = NAME | DESC,
    TERRAIN = 0b100u,
    NAME_TERRAIN = NAME | TERRAIN,
    DESC_TERRAIN = DESC | TERRAIN,
    NAME_DESC_TERRAIN = NAME | DESC | TERRAIN
};

DEFINE_ENUM_COUNT(MaskFlags, 8);

static_assert(static_cast<uint32_t>(MaskFlags::NONE) == 0u, "");
static_assert(static_cast<uint32_t>(MaskFlags::NAME) == 1u, "");
static_assert(static_cast<uint32_t>(MaskFlags::DESC) == 2u, "");
static_assert(static_cast<uint32_t>(MaskFlags::NAME_DESC) == 3u, "");
static_assert(static_cast<uint32_t>(MaskFlags::TERRAIN) == 4u, "");
static_assert(static_cast<uint32_t>(MaskFlags::NAME_TERRAIN) == 5u, "");
static_assert(static_cast<uint32_t>(MaskFlags::DESC_TERRAIN) == 6u, "");
static_assert(static_cast<uint32_t>(MaskFlags::NAME_DESC_TERRAIN) == 7u, "");

static inline constexpr MaskFlags operator&(MaskFlags lhs, MaskFlags rhs)
{
    return static_cast<MaskFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

static MaskFlags getKeyMask(const ParseEvent &event)
{
    assert(event.size() == 3);
    auto cloned = event.clone();
    cloned.reset();
    uint32_t mask = 0;
    for (int i = 0; i < 3; ++i) {
        auto &prop = deref(cloned.next());
        if (!prop.isSkipped())
            mask |= (1u << i);
    }

    assert((mask & 0b111) == mask);

    const auto flags = static_cast<MaskFlags>(mask);
    if (flags == MaskFlags::DESC) {
        // The only one never seen in the wild
        qInfo() << "MaskFlags::DESC observed in the wild!";
    }
    if (flags == MaskFlags::NAME_DESC) {
        // The only one never seen in the wild
        qInfo() << "MaskFlags::NAME_DESC observed in the wild!";
    }
    return flags;
}

static bool isMatchedByTree(const MaskFlags mask)
{
    switch (mask) {
    // case MaskFlags::NAME:      // Not observed in the wild?
    // case MaskFlags::NAME_DESC: // Not observed in the wild?
    case MaskFlags::NAME_DESC_TERRAIN:
        return true;
    default:
        return false;
    }
}

static MaskFlags reduceMask(const MaskFlags mask)
{
    switch (mask) {
    case MaskFlags::NONE:
    case MaskFlags::NAME:
    case MaskFlags::DESC:
    case MaskFlags::DESC_TERRAIN:
    case MaskFlags::TERRAIN:
        return MaskFlags::NONE;

    case MaskFlags::NAME_DESC:
    case MaskFlags::NAME_TERRAIN: // Not supported by tree
        return MaskFlags::NAME;

    case MaskFlags::NAME_DESC_TERRAIN:
        return MaskFlags::NAME_DESC;
    }

    throw std::invalid_argument("mask");
}

static auto makeKey(ParseEvent &event, const MaskFlags maskFlags, const bool verbose)
{
    char buf[64];

    const auto mask = static_cast<uint32_t>(maskFlags);
    assert(event.size() == 3);
    event.reset();
    std::string key{};

    sprintf(buf, "^K%u", mask);
    key += buf;

    for (int i = 0; i < 3; ++i) {
        auto &prop = deref(event.next());
        prop.reset();

        if (!prop.isSkipped() && (mask & (1u << i))) {
            if (verbose) {
                sprintf(buf, ";Property %d:(%zu)[", i, prop.size());
                key.append(buf);
                for (auto c : prop)
                    if (c == ' ')
                        key += '+';
                    else if (std::isprint(c) && !std::isspace(c) && c != '+' && c != '%')
                        key += c;
                    else {
                        sprintf(buf, "%%%02X", c & 0xff);
                        key += buf;
                    }
                key += "]\n";
            } else {
                sprintf(buf, ";P%d:%zu:", i, prop.size());
                key += buf;
                key += prop;
            }
        }
    }

    return key;
}

class ParseHashMap final : public ParseTree::Pimpl
{
private:
    using Key = std::string;
    using PV = SharedRoomCollection;
    using Primary = std::unordered_map<Key, PV>;
    using SV = std::unordered_set<PV>;
    using Secondary = std::unordered_map<Key, SV>;
    Primary m_primary{};
    EnumIndexedArray<Secondary, MaskFlags> m_secondary{};
    const bool m_useVerboseKeys = false;

public:
    explicit ParseHashMap(bool useVerboseKeys = false)
        : m_useVerboseKeys{useVerboseKeys}
    {}

    SharedRoomCollection insertRoom(ParseEvent &event_)
    {
        const auto &event = event_;
        assert(event.size() == 3);
        const MaskFlags mask = getKeyMask(event);

        if (!isMatchedByTree(mask))
            return nullptr;

        auto tmp = event.clone();
        auto pk = makeKey(tmp, MaskFlags::NAME_DESC_TERRAIN, m_useVerboseKeys);
        auto &result = m_primary[pk];
        if (result == nullptr)
            result = std::make_shared<RoomCollection>();

        for (auto subMask = mask; subMask != MaskFlags::NONE; subMask = reduceMask(subMask)) {
            auto key = makeKey(tmp, subMask, m_useVerboseKeys);
            Secondary &reference = m_secondary[subMask];
            SV &bucket = reference[key];
            bucket.emplace(result);
        }

        return result;
    }

    void getRooms(AbstractRoomVisitor &stream, ParseEvent &event_)
    {
        const auto &event = event_;

        assert(event.size() == 3);
        const MaskFlags mask = getKeyMask(event);

        if (!isMatchedByTree(mask))
            return;

        auto tmp = event.clone();
        tmp.reset();

        auto key = makeKey(tmp, mask, m_useVerboseKeys);

        Secondary &thislevel = m_secondary[mask];
        SV &homes = thislevel[key];
        for (const PV &home : homes) {
            if (home != nullptr) {
                home->forEach(stream);
            }
        }

        return;
    }
};

ParseTree::ParseTree(const bool useVerboseKeys)
    : m_pimpl{std::unique_ptr<Pimpl>(static_cast<Pimpl *>(new ParseHashMap(useVerboseKeys)))}
{}
ParseTree::~ParseTree() = default;

SharedRoomCollection ParseTree::insertRoom(ParseEvent &event)
{
    return m_pimpl->insertRoom(event);
}

void ParseTree::getRooms(AbstractRoomVisitor &stream, ParseEvent &event)
{
    m_pimpl->getRooms(stream, event);
}
