#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/IndexedVectorWithDefault.h"
#include "../global/OrderedMap.h"
#include "../global/macros.h"
#include "RawRoom.h"
#include "roomid.h"

#include <ostream>
#include <vector>

class AnsiOstream;
class ProgressCounter;

struct NODISCARD Remapping final
{
private:
    OrderedMap<ExternalRoomId, RoomId> m_extToInt;
    IndexedVectorWithDefault<ExternalRoomId, RoomId> m_intToExt{INVALID_EXTERNAL_ROOMID};

public:
    NODISCARD RoomId convertToInternal(ExternalRoomId ext) const;
    NODISCARD TinyRoomIdSet convertToInternal(const TinyExternalRoomIdSet &ext) const;
    NODISCARD RawExit convertToInternal(const ExternalRawExit &input) const;
    NODISCARD RawRoom convertToInternal(const ExternalRawRoom &input) const;
    NODISCARD std::vector<RawRoom> convertToInternal(const std::vector<ExternalRawRoom> &input) const;

public:
    NODISCARD ExternalRoomId convertToExternal(RoomId id) const;
    NODISCARD TinyExternalRoomIdSet convertToExternal(const TinyRoomIdSet &id) const;
    NODISCARD ExternalRawExit convertToExternal(const RawExit &input) const;
    NODISCARD ExternalRawRoom convertToExternal(const RawRoom &input) const;
    NODISCARD std::vector<ExternalRawRoom> convertToExternal(const std::vector<RawRoom> &input) const;

public:
    NODISCARD static Remapping computeFrom(const std::vector<ExternalRawRoom> &input);

public:
    // WARNING: This is not cheap.
    NODISCARD ExternalRoomId getNextExternal() const;

public:
    NODISCARD size_t size() const { return m_intToExt.size(); }
    NODISCARD bool empty() const { return size() == 0; }
    NODISCARD bool contains(RoomId id) const;

public:
    void resize(size_t size);
    void addNew(RoomId id);
    void undelete(RoomId id, ExternalRoomId extid);
    void removeAt(RoomId id);
    void compact(ProgressCounter &pc, ExternalRoomId firstId);
    void printStats(ProgressCounter &pc, AnsiOstream &os) const;

public:
    NODISCARD bool operator==(const Remapping &rhs) const;
    NODISCARD bool operator!=(const Remapping &rhs) const { return !(rhs == *this); }
};
