#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Charset.h"
#include "../map/CommandId.h"
#include "LineFlags.h"
#include "abstractparser.h"

#include <optional>
#include <string_view>

#include <QByteArray>
#include <QString>
#include <QtCore/QFile>
#include <QtCore>
#include <QtGlobal>

class GmcpMessage;
class GroupManagerApi;
class JsonObj;
class MapData;
class MumeClock;
class ProxyParserApi;
class QFile;
class QObject;
struct TelnetData;

enum class NODISCARD XmlModeEnum : uint8_t {
    NONE,
    ROOM,
    NAME,
    DESCRIPTION,
    EXITS,
    PROMPT,
    TERRAIN,
    HEADER
};

class NODISCARD_QOBJECT MumeXmlParser final : public AbstractParser
{
    Q_OBJECT

private:
    XmlModeEnum m_xmlMode = XmlModeEnum::NONE;
    LineFlags m_lineFlags;
    QString m_lineToUser;
    QString m_tempCharacters;
    QString m_tempTag;
    QString m_stringBuffer;
    CommandEnum m_move = CommandEnum::NONE;
    ServerRoomId m_serverId = INVALID_SERVER_ROOMID;
    std::optional<RoomId> m_expectedMove;
    std::optional<ServerExitIds> m_exitIds;
    bool m_readingTag = false;
    bool m_gratuitous = false;
    bool m_exitsReady = false;
    bool m_descriptionReady = false;

    // REVISIT: Is there any point to having a distinction between null and empty?
    std::optional<RoomName> m_roomName;
    std::optional<RoomDesc> m_roomDesc;
    std::optional<RoomContents> m_roomContents;

private:
    enum class NODISCARD XmlAttributeStateEnum : uint8_t {
        /// received <room/>
        ELEMENT,
        /// received <room terrain/>
        ATTRIBUTE,
        /// received <room terrain=/>
        EQUALS,
        /// received <room terrain=field/>
        UNQUOTED_VALUE,
        /// received <room terrain='field'/>
        SINGLE_QUOTED_VALUE,
        /// received <room terrain="field"/>
        DOUBLE_QUOTED_VALUE
    };

public:
    explicit MumeXmlParser(
        MapData &, MumeClock &, ProxyParserApi, GroupManagerApi, CTimers &timers, QObject *parent);
    ~MumeXmlParser() final;

private:
    void parse(const TelnetData &, bool isGoAhead);

public:
    void slot_parseNewMudInput(const TelnetData &data);
    void slot_parseGmcpInput(const GmcpMessage &msg);

private:
    void parseMudCommands(const QString &str);
    NODISCARD QString characters(QString &ch);
    NODISCARD bool element(const QString &);
    void maybeUpdate(RoomId expectedId, const ParseEvent &ev);
    void setMove(CommandEnum dir);
    void move();
    void parseGmcpStatusVars(const JsonObj &obj);
    void parseGmcpCharVitals(const JsonObj &obj);
    void parseGmcpEventMoved(const JsonObj &obj);
    void parseGmcpRoomInfo(const JsonObj &obj);
};
