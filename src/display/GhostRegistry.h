#pragma once
#include "../map/roomid.h" // ServerRoomId

#include <unordered_map>

#include <QString>

struct GhostInfo
{
    QString tokenKey; // icon to draw (display name)
};

extern std::unordered_map<ServerRoomId, GhostInfo> g_ghosts;
