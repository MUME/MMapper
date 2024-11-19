// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Remapping.h"

#include "../global/AnsiOstream.h"
#include "../global/Timer.h"
#include "../global/logging.h"
#include "../global/progresscounter.h"
#include "InvalidMapOperation.h"
#include "RawExit.h"
#include "RawRoom.h"

#include <ostream>
#include <set>
#include <vector>

template<typename Result, typename Input, typename Convert>
NODISCARD static Result convertExit(const Input &input, Convert &&convert)
{
    Result result;
    result.fields = input.fields;
    result.outgoing = convert(input.outgoing);
    result.incoming = convert(input.incoming);
    return result;
}

template<typename Result, typename Input, typename Convert>
NODISCARD static Result convertRoom(const Input &input, Convert &&convert)
{
    Result result;
    result.fields = input.fields;
    for (const ExitDirEnum dir : ALL_EXITS7) {
        result.exits[dir] = convert(input.exits[dir]);
    }
    result.position = input.position;
    result.id = convert(input.id);
    result.server_id = input.server_id;
    result.status = input.status;
    return result;
}

NODISCARD RoomId Remapping::convertToInternal(const ExternalRoomId ext) const
{
    if (const auto it = m_extToInt.find(ext)) {
        return *it;
    }
    return INVALID_ROOMID;
}

TinyRoomIdSet Remapping::convertToInternal(const TinyExternalRoomIdSet &set) const
{
    TinyRoomIdSet replacement;
    for (const ExternalRoomId id : set) {
        replacement.insert(convertToInternal(id));
    }
    return replacement;
}

RawExit Remapping::convertToInternal(const ExternalRawExit &input) const
{
    return convertExit<RawExit, ExternalRawExit>(input, [this](const auto &x) {
        return convertToInternal(x);
    });
}

RawRoom Remapping::convertToInternal(const ExternalRawRoom &input) const
{
    return convertRoom<RawRoom, ExternalRawRoom>(input, [this](const auto &x) {
        return convertToInternal(x);
    });
}

std::vector<RawRoom> Remapping::convertToInternal(const std::vector<ExternalRawRoom> &input) const
{
    DECL_TIMER(t, "convertToInternal");
    std::vector<RawRoom> result;
    result.reserve(input.size());
    for (const auto &room : input) {
        result.emplace_back(convertToInternal(room));
    }
    return result;
}

NODISCARD ExternalRoomId Remapping::convertToExternal(const RoomId id) const
{
    const uint32_t pos = id.asUint32();
    if (pos < m_intToExt.size()) {
        return m_intToExt.at(id);
    }
    return INVALID_EXTERNAL_ROOMID;
}

TinyExternalRoomIdSet Remapping::convertToExternal(const TinyRoomIdSet &set) const
{
    TinyExternalRoomIdSet replacement;
    for (const RoomId id : set) {
        replacement.insert(convertToExternal(id));
    }
    return replacement;
}

ExternalRawExit Remapping::convertToExternal(const RawExit &input) const
{
    return convertExit<ExternalRawExit, RawExit>(input, [this](const auto &x) {
        return convertToExternal(x);
    });
}

ExternalRawRoom Remapping::convertToExternal(const RawRoom &input) const
{
    return convertRoom<ExternalRawRoom, RawRoom>(input, [this](const auto &x) {
        return convertToExternal(x);
    });
}

std::vector<ExternalRawRoom> Remapping::convertToExternal(const std::vector<RawRoom> &input) const
{
    DECL_TIMER(t, "convertToExternal");
    std::vector<ExternalRawRoom> result;
    result.reserve(input.size());
    for (const auto &room : input) {
        result.emplace_back(convertToExternal(room));
    }
    return result;
}

Remapping Remapping::computeFrom(const std::vector<ExternalRawRoom> &input)
{
    if (input.empty()) {
        return Remapping{};
    }

    DECL_TIMER(t, "building RoomId mapping");
    Remapping remapping;

    std::set<ExternalRoomId> seen;
    for (const auto &r : input) {
        seen.insert(r.id);
        for (const auto &e : r.exits) {
            for (const ExternalRoomId to : e.outgoing) {
                seen.insert(to);
            }
            for (const ExternalRoomId from : e.incoming) {
                seen.insert(from);
            }
        }
    }

    if ((false)) {
        MMLOG() << "# of unique roomids = " << seen.size();
        MMLOG() << "lowest room id = " << seen.begin()->value();
        MMLOG() << "highest room id = " << seen.rbegin()->value();
    }

    // remapping.m_intToExt.reserve(seen.size());

    std::vector<ExternalRoomId> intToExt;
    RoomId next{0};
    for (const auto &r : seen) {
        assert(next.value() == intToExt.size());
        const ExternalRoomId ext{r};
        remapping.m_extToInt.set(ext, next);
        intToExt.push_back(ext);
        next = next.next();
    }

    remapping.m_intToExt.init(intToExt.data(), intToExt.size());

    assert(next.asUint32() == seen.size());
    assert(next.asUint32() == remapping.m_intToExt.size());
    assert(next.asUint32() == remapping.m_extToInt.size());

    for (const auto &kv : remapping.m_extToInt) {
        assert(remapping.m_intToExt.at(kv.second) == kv.first);
    }

    return remapping;
}

ExternalRoomId Remapping::getNextExternal() const
{
    if (m_extToInt.empty()) {
        return ExternalRoomId{0};
    }

    const auto any = m_extToInt.begin()->first;
    ExternalRoomId highest = any;

    // There's currently no iterator, so we have to check the values manually.
    const auto size = m_intToExt.size();
    for (uint32_t i = 0; i < size; ++i) {
        const auto x = m_intToExt.at(RoomId{i});
        if (x != INVALID_EXTERNAL_ROOMID) {
            if (x > highest) {
                highest = x;
            }
        }
    }
    return highest.next();
}

bool Remapping::contains(const RoomId id) const
{
    if (id == INVALID_ROOMID) {
        return false;
    }
    const auto pos = id.asUint32();
    if (pos >= m_intToExt.size()) {
        return false;
    }
    return m_intToExt.at(id) != INVALID_EXTERNAL_ROOMID;
}

void Remapping::resize(const size_t size)
{
    const auto from = m_intToExt.size();
    assert(size > from);

    m_intToExt.grow_to_size(size);

    assert(m_intToExt.size() == size);
}

void Remapping::addNew(const RoomId id)
{
    assert(!contains(id));
    const auto nextExternal = getNextExternal();
    const auto pos = id.asUint32();
    assert(pos >= m_intToExt.size() || m_intToExt.at(id) == INVALID_EXTERNAL_ROOMID);
    assert(m_extToInt.find(nextExternal) == nullptr);
    if (pos >= m_intToExt.size()) {
        resize(pos + 1);
    }
    m_intToExt.set(id, nextExternal);
    m_extToInt.set(nextExternal, id);
    assert(contains(id));
}

void Remapping::undelete(const RoomId id, const ExternalRoomId extid)
{
    assert(!contains(id));
    assert(m_extToInt.find(extid) == nullptr);
    const auto pos = id.asUint32();
    assert(pos >= m_intToExt.size() || m_intToExt.at(id) == INVALID_EXTERNAL_ROOMID);
    assert(m_extToInt.find(extid) == nullptr);
    if (pos >= m_intToExt.size()) {
        resize(pos + 1);
    }
    m_intToExt.set(id, extid);
    m_extToInt.set(extid, id);
    assert(contains(id));
}

void Remapping::removeAt(const RoomId id)
{
    if (id == INVALID_ROOMID) {
        throw InvalidMapOperation();
    }

    const ExternalRoomId ext = m_intToExt.at(id);
    m_intToExt.set(id, INVALID_EXTERNAL_ROOMID);

    if (ext != INVALID_EXTERNAL_ROOMID) {
        m_extToInt.erase(ext);
    }

    assert(!contains(id));
}

void Remapping::compact(ProgressCounter &pc, const ExternalRoomId firstId)
{
    if (firstId == INVALID_EXTERNAL_ROOMID) {
        throw InvalidMapOperation();
    }

    ExternalRoomId next = firstId;
    OrderedMap<ExternalRoomId, RoomId> newExtToInt;

    pc.increaseTotalStepsBy(m_extToInt.size());
    for (const auto &kv : m_extToInt) {
        m_intToExt.set(kv.second, next);
        newExtToInt.set(next, kv.second);
        next = next.next();
        pc.step();
    }
    m_extToInt = newExtToInt;
}

void Remapping::printStats(ProgressCounter & /*pc*/, AnsiOstream &os) const
{
    RoomId lo = INVALID_ROOMID;
    RoomId hi = INVALID_ROOMID;
    ExternalRoomId extLo = INVALID_EXTERNAL_ROOMID;
    ExternalRoomId extHi = INVALID_EXTERNAL_ROOMID;

    if (!m_extToInt.empty()) {
        lo = RoomId{0};
        hi = RoomId{static_cast<uint32_t>(m_intToExt.size() - 1u)};

        for (; lo <= hi; lo = lo.next()) {
            if (m_intToExt[lo] != INVALID_EXTERNAL_ROOMID) {
                break;
            }
        }
        for (; lo < hi; hi = RoomId{hi.value() - 1}) {
            if (m_intToExt[hi] != INVALID_EXTERNAL_ROOMID) {
                break;
            }
        }

        const auto any = m_extToInt.begin()->first;
        extLo = any;
        extHi = any;
        for (auto it = lo; it <= hi; it = it.next()) {
            auto ext = m_intToExt.at(it);
            if (ext < extLo) {
                extLo = ext;
            }
            if (ext > extHi) {
                extHi = ext;
            }
        }
    }

    auto print = [&os](std::string_view prefix, size_t size, uint32_t loval, uint32_t hival) {
        os << prefix << size;
        if (loval != UINT_MAX && hival != UINT_MAX) {
            os << " (" << loval << " to " << hival << ")";
        }
        os << ".\n";
    };

    print("Allocated internal IDs: ", m_intToExt.size(), lo.value(), hi.value());
    print("Allocated external IDs: ", m_extToInt.size(), extLo.value(), extHi.value());
}

bool Remapping::operator==(const Remapping &rhs) const
{
    return m_extToInt == rhs.m_extToInt     //
           && m_intToExt == rhs.m_intToExt; //
}
