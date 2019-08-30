// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ParseTree.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/property.h"
#include "../global/Array.h"
#include "../global/utils.h"
#include "roomcollection.h"

enum class MaskFlagsEnum : uint32_t {
    NONE = 0u,
    NAME = 0b001u,
    DESC = 0b010u,
    NAME_DESC = NAME | DESC,
    TERRAIN = 0b100u,
    NAME_TERRAIN = NAME | TERRAIN,
    DESC_TERRAIN = DESC | TERRAIN,
    NAME_DESC_TERRAIN = NAME | DESC | TERRAIN
};

DEFINE_ENUM_COUNT(MaskFlagsEnum, 8)

static_assert(static_cast<uint32_t>(MaskFlagsEnum::NONE) == 0u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::NAME) == 1u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::DESC) == 2u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::NAME_DESC) == 3u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::TERRAIN) == 4u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::NAME_TERRAIN) == 5u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::DESC_TERRAIN) == 6u);
static_assert(static_cast<uint32_t>(MaskFlagsEnum::NAME_DESC_TERRAIN) == 7u);

static MaskFlagsEnum getKeyMask(const ParseEvent &event)
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

    const auto flags = static_cast<MaskFlagsEnum>(mask);
    if (flags == MaskFlagsEnum::DESC) {
        // The only one never seen in the wild
        static std::once_flag flag;
        std::call_once(flag, []() {
            std::cerr << "WARNING: MaskFlagsEnum::DESC observed in the wild!" << std::endl;
            assert(false);
        });
    }
    return flags;
}

static bool isMatchedByTree(const MaskFlagsEnum mask)
{
    switch (mask) {
    case MaskFlagsEnum::NAME:      // Not observed in the wild?
    case MaskFlagsEnum::NAME_DESC: // Offline movement
    case MaskFlagsEnum::NAME_DESC_TERRAIN:
        return true;

    case MaskFlagsEnum::NONE:
    case MaskFlagsEnum::DESC:
    case MaskFlagsEnum::TERRAIN:
    case MaskFlagsEnum::NAME_TERRAIN:
    case MaskFlagsEnum::DESC_TERRAIN:
    default:
        return false;
    }
}

static MaskFlagsEnum reduceMask(const MaskFlagsEnum mask)
{
    switch (mask) {
    case MaskFlagsEnum::NONE:
    case MaskFlagsEnum::NAME:
    case MaskFlagsEnum::DESC:
    case MaskFlagsEnum::DESC_TERRAIN:
    case MaskFlagsEnum::TERRAIN:
        return MaskFlagsEnum::NONE;

    case MaskFlagsEnum::NAME_DESC:
    case MaskFlagsEnum::NAME_TERRAIN: // Not supported by tree
        return MaskFlagsEnum::NAME;

    case MaskFlagsEnum::NAME_DESC_TERRAIN:
        return MaskFlagsEnum::NAME_DESC;
    }

    throw std::invalid_argument("mask");
}

static auto makeKey(ParseEvent &event, const MaskFlagsEnum maskFlags)
{
    char buf[64];

    const auto mask = static_cast<uint32_t>(maskFlags);
    assert(event.size() == 3);
    event.reset();
    std::string key;

    std::snprintf(buf, sizeof(buf), "^K%u", mask);
    key += buf;

    for (int i = 0; i < 3; ++i) {
        auto &prop = deref(event.next());
        prop.reset();

        if (!prop.isSkipped() && (mask & (1u << i))) {
            std::snprintf(buf, sizeof(buf), ";P%d:%zu:", i, prop.size());
            key += buf;
            key += prop;
        }
    }

    return key;
}

class ParseTree::ParseHashMap final
{
private:
    using Key = std::string;
    using PV = SharedRoomCollection;
    using Primary = std::unordered_map<Key, PV>;
    using SV = std::unordered_set<PV>;
    using Secondary = std::unordered_map<Key, SV>;
    Primary m_primary;
    EnumIndexedArray<Secondary, MaskFlagsEnum> m_secondary;

public:
    ParseHashMap() = default;
    virtual ~ParseHashMap();

    SharedRoomCollection insertRoom(const ParseEvent &event)
    {
        assert(event.size() == 3);
        const MaskFlagsEnum mask = getKeyMask(event);

        if (!isMatchedByTree(mask))
            return nullptr;

        auto tmp = event.clone();
        auto pk = makeKey(tmp, MaskFlagsEnum::NAME_DESC_TERRAIN);
        auto &result = m_primary[pk];
        if (result == nullptr)
            result = std::make_shared<RoomCollection>();

        for (auto subMask = mask; subMask != MaskFlagsEnum::NONE; subMask = reduceMask(subMask)) {
            auto key = makeKey(tmp, subMask);
            Secondary &reference = m_secondary[subMask];
            SV &bucket = reference[key];
            bucket.emplace(result);
        }

        return result;
    }

    void getRooms(AbstractRoomVisitor &stream, const ParseEvent &event)
    {
        assert(event.size() == 3);
        const MaskFlagsEnum mask = getKeyMask(event);

        if (!isMatchedByTree(mask))
            return;

        auto tmp = event.clone();
        tmp.reset();

        auto key = makeKey(tmp, mask);

        Secondary &thislevel = m_secondary[mask];
        SV &homes = thislevel[key];
        for (const PV &home : homes) {
            if (home != nullptr) {
                home->forEach(stream);
            }
        }
    }
};

ParseTree::ParseHashMap::~ParseHashMap() = default;

ParseTree::ParseTree()
    : m_pimpl{std::make_unique<ParseHashMap>()}
{}
ParseTree::~ParseTree() = default;

SharedRoomCollection ParseTree::insertRoom(const ParseEvent &event)
{
    return m_pimpl->insertRoom(event);
}

void ParseTree::getRooms(AbstractRoomVisitor &stream, const ParseEvent &event)
{
    m_pimpl->getRooms(stream, event);
}
