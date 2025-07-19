#pragma once
#include <unordered_map>
#include <QString>


#include "../map/roomid.h"
#include "../group/CGroupChar.h"

struct GhostInfo {
    ServerRoomId roomSid;
    QString      tokenKey;
};

extern std::unordered_map<GroupId, GhostInfo> g_ghosts;
