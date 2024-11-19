// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "metatypes.h"

#include "../expandoracommon/parseevent.h"
#include "../global/ConfigEnums.h"
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/roomselection.h"
#include "../mpi/remoteeditsession.h"
#include "../pandoragroup/groupauthority.h"
#include "../pandoragroup/mmapper2character.h"
#include "../parser/CommandQueue.h"
#include "../parser/DoorAction.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/telnetfilter.h"

#include <type_traits>

#include <qmetatype.h>

void registerMetatypes()
{
#define REGISTER_METATYPE(T) \
    do { \
        /* Qt has these requirements, but the error messages are confusing. */ \
        static_assert(std::is_default_constructible_v<T>, \
                      #T " must be default constructible to be a metatype"); \
        static_assert(std::is_copy_constructible_v<T>, \
                      #T " must be copy constructible to be a metatype"); \
        qDebug().noquote().nospace() \
            << "Registering metatype " << #T << " at " << __FILE__ << ":" << __LINE__; \
        qRegisterMetaType<T>(#T); \
    } while (false)

    REGISTER_METATYPE(RoomId);
    REGISTER_METATYPE(TelnetData);
    REGISTER_METATYPE(CommandQueue);
    REGISTER_METATYPE(DoorActionEnum);
    REGISTER_METATYPE(ExitDirEnum);
    REGISTER_METATYPE(GroupManagerStateEnum);
    REGISTER_METATYPE(SigParseEvent);
    REGISTER_METATYPE(SigRoomSelection);
    REGISTER_METATYPE(CharacterAffectEnum);
    REGISTER_METATYPE(CharacterPositionEnum);
    REGISTER_METATYPE(GmcpMessage);
    REGISTER_METATYPE(GroupSecret);
    REGISTER_METATYPE(RemoteSession);

#undef REGISTER_METATYPE
}
