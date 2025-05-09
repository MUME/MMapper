#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/macros.h"
#include "AbstractChangeVisitor.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"
#include "Map.h"
#include "RoomHandle.h"
#include "mmapper2room.h"
#include "roomid.h"

#include <iosfwd>
#include <ostream>

struct NODISCARD ChangePrinter final : public AbstractChangeVisitor
{
    using Remap = std::function<ExternalRoomId(RoomId)>;

private:
    struct StructHelper;
    struct FlagsHelper;
    friend StructHelper;
    friend FlagsHelper;
    const Remap m_remap;
    AnsiOstream &m_os;

public:
    explicit ChangePrinter(Remap map, AnsiOstream &_os);
    ~ChangePrinter() final;

private:
#ifndef NDEBUG
    NORETURN
#endif
    void error();

private:
    void print(bool val);
    void print(ExitDirEnum dir);
    void print(RoomId room);
    void print(ExternalRoomId room);

private:
    void print(ServerRoomId serverId);
    void print(const Coordinate &coord);

private:
    void print(const DoorName &name);
    void print(const RoomContents &name);
    void print(const RoomName &name);
    void print(const RoomNote &name);
    void print(const RoomDesc &name);

private:
    void print(ChangeTypeEnum type);
    void print(DirectSunlightEnum type);
    void print(DoorFlagEnum flag);
    void print(ExitFlagEnum flag);
    void print(FlagChangeEnum type);
    void print(FlagModifyModeEnum mode);
    void print(PositionChangeEnum type);
    void print(PromptFogEnum type);
    void print(PromptWeatherEnum type);
    void print(RoomAlignEnum type);
    void print(RoomLightEnum type);
    void print(RoomLoadFlagEnum flag);
    void print(RoomMobFlagEnum flag);
    void print(RoomPortableEnum type);
    void print(RoomRidableEnum type);
    void print(RoomSundeathEnum type);
    void print(RoomTerrainEnum type);
    void print(WaysEnum ways);

private:
    void print(ConnectedRoomFlagsType flags);
    void print(ExitsFlagsType flags);
    void print(DoorFlags doorFlags);
    void print(ExitFlags doorFlags);
    void print(PromptFlagsType flags);
    void print(RoomLoadFlags loadFlags);
    void print(RoomMobFlags mobFlags);

private:
    void print(const ExitFieldVariant &var);
    void print(const RoomFieldVariant &var);

private:
    void print(const RoomIdSet &set);

private:
    void print(const ParseEvent &ev);

private:
#define X_NOP()
#define X_DECL_VIRT_ACCEPT(_Type) void virt_accept(const _Type &) override;
    XFOREACH_CHANGE_TYPE(X_DECL_VIRT_ACCEPT, X_NOP)
#undef X_DECL_VIRT_ACCEPT
#undef X_NOP

public:
    void print(const Change &change)
    {
        this->accept(change);
    }
};
