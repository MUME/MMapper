// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "metatypes.h"

#include "../global/ConfigEnums.h"
#include "../map/ChangeList.h"
#include "../map/ExitDirection.h"
#include "../map/parseevent.h"
#include "../map/roomid.h"
#include "../mapdata/roomselection.h"
#include "../mpi/remoteeditsession.h"
#include "../pandoragroup/groupauthority.h"
#include "../pandoragroup/mmapper2character.h"
#include "../parser/CommandQueue.h"
#include "../parser/DoorAction.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/telnetfilter.h"
#include "./proxy/TaggedBytes.h"

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
    REGISTER_METATYPE(ExternalRoomId);
    REGISTER_METATYPE(ServerRoomId);
    REGISTER_METATYPE(SigMapChangeList);
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

#define X_REGISTER_METATYPE(name) REGISTER_METATYPE(name##Bytes);
    XFOREACH_TAGGED_BYTE_TYPES(X_REGISTER_METATYPE)
#undef X_REGISTER_METATYPE

#undef REGISTER_METATYPE
}
