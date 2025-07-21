#pragma once
#include <unordered_map>
#include <QString>
#include "../map/roomid.h"         // ServerRoomId

struct GhostInfo {
    QString tokenKey;             // icon to draw (display name)
};

extern std::unordered_map<ServerRoomId, GhostInfo> g_ghosts;
